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
    HANDLE lockoutput;
#else
    pthread_mutex_t lock;
    pthread_mutex_t lockOutput;
#endif

char g_working = 0, g_workersUsed = 0;

unsigned long g_id = 0;

unsigned long g_startPrev = 0, g_start = 0, g_length = 0, g_end = 0, g_batch;
unsigned char g_pwd[1024*1027], g_difficulty[34];
unsigned char pwd[1024*1027], difficulty[34];

unsigned char g_bestHash[34];
unsigned long g_bestHashNonce = 0;
unsigned long g_hashesTotal = 0;

unsigned char g_cores = 4;

unsigned char g_debug = 1;

char * g_filename;
char g_filenameOutput[50] ;

clock_t g_tstart;


int readData(char * filename){

    int i, a, ok;
    unsigned long _start, _length, security;


    if (fileExists(filename) == 0) return 0;

    FILE *fin = fopen(filename, "rb");

    fscanf(fin, "%lu", &_start);
    fscanf(fin, "%lu", &_length);

    if (_start == 0 && _length == 0) {
        fclose(fin);
        return -1;
    }

#ifdef WIN32
    WaitForSingleObject(lock, INFINITE);
#else
    pthread_mutex_lock(&lock);
#endif


    //printf("hash:   \n");
    for (i = 0; i < _length; i++) {
        fscanf(fin, "%hhu", &pwd[i] );

        if (feof(fin)) {
            fclose(fin);
#ifdef WIN32
            ReleaseMutex(lock);
#else
            pthread_mutex_unlock(&lock);
#endif
            return 0;
        }
        //std::cout << pwd[i] << " ";
    }

    //printf("DIFFICULTY:   \n");
    for (i = 0; i < 32; i++) {

        fscanf(fin, "%hhu", &difficulty[i]);

        if (feof(fin)) {
            fclose(fin);
#ifdef WIN32
            ReleaseMutex(lock);
#else
            pthread_mutex_unlock(&lock);
#endif
            return 0;
        }
        //std::cout << pwd[i] << " ";
    }

    //fin >> pwdHex;
    //fin >> difficultyHex;
    fscanf(fin, "%lu", &g_end);
    fscanf(fin, "%lu", &g_batch);

    fscanf(fin, "%lu", &security);

    //std::cout  << " cool " << end << " " << batch << " " << security << "\n";

    if (security != 218391) {
        fclose(fin);
#ifdef WIN32
        ReleaseMutex(lock);
#else
        pthread_mutex_unlock(&lock);
#endif
        return 0;
    }


    //check if it identical
    ok = 1;
    if (_start == g_startPrev && _length == g_length) {

        for (i=0; i < _length; i++)
            if (pwd[i] != g_pwd[i]){
                ok = 0;
                break;
            }

        for (i=0; i < 32; i++)
            if (difficulty[i] != g_difficulty[i]){
                ok = 0;
                break;
            }

        if (ok == 1){
            fclose(fin);
#ifdef WIN32
            ReleaseMutex(lock);
#else
            pthread_mutex_unlock(&lock);
#endif
            return 0;
        }
    }

    for (i=0; i < _length; i++)
        g_pwd[i] = pwd[i];

    for (i=0; i < 32; i++)
        g_difficulty[i] = difficulty[i];



    g_start = _start;
    g_startPrev = g_start;
    g_length = _length;

    for (i = 0; i < 32; i++)
        g_bestHash[i] = 255;

    g_bestHashNonce = 0;
    g_hashesTotal = 0;
    g_tstart = clock();

    g_id++;
    g_workersUsed = 0;

    fclose(fin);
#ifdef WIN32
    ReleaseMutex(lock);
#else
    pthread_mutex_unlock(&lock);
#endif

    if (g_debug)
        printf("DATA READ!!! %lu %lu %lu \n", g_length, g_start, g_end);

    /*
        for (i=0;i < length;i++) {
             char a = pwdHex[2 * i],  b = pwdHex[2 * i + 1];
            pwd[i] = (((encode(a) * 16) & 0xF0) + (encode(b) & 0x0F));
        }

        for (i=0;i < 32; i++) {
             char a = difficultyHex[2 * i],  b = difficultyHex[2 * i + 1];
         difficulty[i] = (((encode(a) * 16) & 0xF0) + (encode(b) & 0x0F));

        std::cout << length << " " << start << " "<< end << " " << batch << '\n';
        std::cout << pwd << '\n';
        std::cout << difficulty << '\n';

    */

    /*
        for (auto q=0; q < length; q++){
             d2base( pwd[q], 16);
             std::cout << " ";
        }

        std::cout << "\n\n";
       for (auto q=0; q < 32; q++){
             //d2base( difficulty[q], 16);
             //std::cout << difficulty[q];
                 std::cout << " ";
            }
            std::cout  << "\n\n";
        */


    return 1;

}



#endif