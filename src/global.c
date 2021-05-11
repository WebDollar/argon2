#ifndef GLOBAL2_H
#define GLOBAL2_H

#include "stdio.h"
#include "time.h"
#include "unistd.h"

#ifdef WIN32
    #include <windows.h>
#else
    #include<pthread.h>
#endif

char arr[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};



int fileExists (char * filename) {

//    FILE *file  = fopen( filename, "r");
//
//    if ( file ) {
//        fclose(file);
//        return 1;
//    }
//    return 0;

    if( access( filename, F_OK ) != -1 ) {
        return 1;
        // file exists
    } else {
        return 0;
        // file doesn't exist
    }
}

#ifdef WIN32
    HANDLE lock;
    HANDLE lockOutput;
#else
    pthread_mutex_t lock;
    pthread_mutex_t lockOutput;
#endif

char g_working = 0, g_workersUsed = 0;

unsigned long g_id = 0;

unsigned long g_startPrev = 0, g_start = 0, g_length = 0, g_end = 0, g_batch = 0;
unsigned char g_pwd[1024*1027], g_difficulty[34];

unsigned char dbl_pwd[1024*1027], dbl_difficulty[34];
unsigned long dbl_end, dbl_batch;
// ^ naming these dbl_ like "double-buffer"

unsigned char g_bestHash[34];
unsigned long g_bestHashNonce = 0;
unsigned long g_hashesTotal = 0;

unsigned char g_cores = 4;

unsigned char g_debug = 1;

char * g_filename;
char g_filenameOutput[1024] ;

clock_t g_tstart;

#define readdata_debug(...) fprintf(rdDebugOut, __VA_ARGS__)
#define readdata_flush() fflush(rdDebugOut)
//FILE *rdDebugOut;
//char rdDebugFilename[512];


// Return type: Integer; values:
// -1 = All done, shut down the whole argon2 process (main will exit)
//  0 = No new data to process yet
//  1 = New data to process!
int readData(char * filename){

    int i, a, is_identical;
    unsigned long _start, _length, security;


    if (fileExists(filename) == 0) return 0;

    FILE *fin = fopen(filename, "rb");
    if (!fin) {
        //readdata_debug("Error opening input file %s; return 0\n", filename);
        //readdata_flush();
        return 0;
    }

    if (EOF == fscanf(fin, "%lu", &_start)) { fclose(fin); return 0; }
    if (EOF == fscanf(fin, "%lu", &_length)) { fclose(fin); return 0; }

    if (_start == 0 && _length == 0) {
        fclose(fin);
        return -1;
    }

    for (i = 0; i < _length; i++) {
        if (EOF == fscanf(fin, "%hhu", &dbl_pwd[i] )) { fclose(fin); return 0; }

        if (feof(fin)) { fclose(fin); return 0; }
    }

    for (i = 0; i < 32; i++) {

        if (EOF == fscanf(fin, "%hhu", &dbl_difficulty[i])) { fclose(fin); return 0; }

        if (feof(fin)) { fclose(fin); return 0; }
    }

    if (EOF == fscanf(fin, "%lu", &dbl_end)) { fclose(fin); return 0; }
    if (EOF == fscanf(fin, "%lu", &dbl_batch)) { fclose(fin); return 0; }

    if (EOF == fscanf(fin, "%lu", &security)) { fclose(fin); return 0; }

    if (security != 218391) { fclose(fin); return 0; }

	fclose(fin);  // All done with the input file

//    if (_start != g_startPrev) {
//    	readdata_debug("Loaded record: start: %lu; length: %lu; end: %lu; batch: %lu\n", _start, _length, dbl_end, dbl_batch);
//    	readdata_flush();
//    }

    // File is valid; copy contents into g_pwd[] and g_difficulty[] after locking mutex
#ifdef WIN32
    WaitForSingleObject(lock, INFINITE);
#else
    pthread_mutex_lock(&lock);
#endif

    //check if this dataset is identical to the one we're already processing
    // As we're comparing data against g_pwd/g_difficulty which is accessed by the threads, this should be done
    // under mutex protection. (maybe not 100% necessary since the threads don't write to these arrays?)
    is_identical = 1;
    if (_start == g_startPrev && _length == g_length) {

        for (i=0; i < _length; i++)
            if (dbl_pwd[i] != g_pwd[i]){
                is_identical = 0;
                break;
            }

        for (i=0; i < 32; i++)
            if (dbl_difficulty[i] != g_difficulty[i]){
                is_identical = 0;
                break;
            }

        if (is_identical == 1){
#ifdef WIN32
            ReleaseMutex(lock);
#else
            pthread_mutex_unlock(&lock);
#endif
            //readdata_debug("^ Data was identical to existing processing dataset though; return 0\n");
            //readdata_flush();
            return 0;  // Still not worried about processing this one
        }
    }

	// Transfer file-sourced block data & difficulty into the active datasets
    for (i=0; i < _length; i++)
        g_pwd[i] = dbl_pwd[i];

    for (i=0; i < 32; i++)
        g_difficulty[i] = dbl_difficulty[i];


    // reset g_start to beginning of this new work;
    // keep in mind g_start is modified by threads as they consume batches of nonces
    g_start = _start;
    g_startPrev = _start;  // indicates the _start from the last updated file/dataset
    g_length = _length;    // Length of data (block including transactions)
    g_end = dbl_end;       // the very last nonce to attempt for this dataset
    g_batch = dbl_batch;   // incremental batch of nonces for each worker thread to search

	// Reset best-hash/nonce parameters
    for (i = 0; i < 32; i++)
        g_bestHash[i] = 255;

    g_bestHashNonce = 0;
    g_hashesTotal = 0;
    g_tstart = clock();

    // Incremental ID to distinguish one dataset from another;
    // threads use this to discover if they're working against stale data
    g_id++;
    // Counter to express how many batches of nonces have been tried by different threads for this particular dataset
    g_workersUsed = 0;

#ifdef WIN32
    ReleaseMutex(lock);
#else
    pthread_mutex_unlock(&lock);
#endif

    //readdata_debug("Committed new dataset; g_start: %lu; g_length: %lu; g_id: %lu\n", g_start, g_length, g_id);
    //readdata_flush();

    return 1;

}



#endif
