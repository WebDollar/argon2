/*
 * Argon2 reference source code package - reference C implementations
 *
 * Copyright 2015
 * Daniel Dinu, Dmitry Khovratovich, Jean-Philippe Aumasson, and Samuel Neves
 *
 * You may use this work under the terms of a Creative Commons CC0 1.0
 * License/Waiver or the Apache Public License 2.0, at your option. The terms of
 * these licenses can be found at:
 *
 * - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
 * - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0
 *
 * You should have received a copy of both of these licenses along with this
 * software. If not, they may be obtained at the above URLs.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "argon2.h"

#include "global.c"

#ifdef WIN32
    #include <windows.h>
    HANDLE tid[100];
#else
    #include<pthread.h>
    #include<unistd.h>
    pthread_t tid[100];
#endif

static uint64_t rdtsc(void) {
#ifdef _MSC_VER
    return __rdtsc();
#else
#if defined(__amd64__) || defined(__x86_64__)
    uint64_t rax, rdx;
    __asm__ __volatile__("rdtsc" : "=a"(rax), "=d"(rdx) : :);
    return (rdx << 32) | rax;
#elif defined(__i386__) || defined(__i386) || defined(__X86__)
    uint64_t rax;
    __asm__ __volatile__("rdtsc" : "=A"(rax) : :);
    return rax;
#else
#error "Not implemented!"
#endif
#endif
}

#define CONST_CAST(x) (x)(uintptr_t)




argon2_type type = 0; //argon2_type.Argon2_d;


#define bench_debug(...) fprintf(benchDebugOut, __VA_ARGS__)
#define bench_flush() fflush(benchDebugOut)

/*
 * Benchmarks Argon2 with salt length 16, password length 16, t_cost 3,
   and different m_cost and threads
 */

#ifdef WIN32
DWORD WINAPI benchmark(void* data) {
#else
void * benchmark() {
#endif

    //FILE *benchDebugOut;
    //char benchDebugFilename[512];

    clock_t tstop;
    char change, solution;
    unsigned char out[32];
    FILE * fout;

    uint32_t length;
    unsigned char pwd[1024*1024+5];
    //unsigned char pwdBuffer[1024*1024+5];

    unsigned char target[33], bestHash[33];
    unsigned long i;

    unsigned long j;
    unsigned long start = 0;
    unsigned long end = 0;
    unsigned long bestHashNonce;

    unsigned char hash[65];

    unsigned long idPrev = 0;



    argon2_context context;
    int result;

    context.out = (uint8_t *)out;
    context.outlen = (uint32_t)32;
    context.salt = CONST_CAST(uint8_t *)"Satoshi_is_Finney";
    context.saltlen = (uint32_t)17;
    context.secret = NULL;
    context.secretlen = 0;
    context.ad = NULL;
    context.adlen = 0;
    context.t_cost = 2;
    context.m_cost = 256;
    context.lanes = 2;
    context.threads = 1;
    context.allocate_cbk = NULL;
    context.free_cbk = NULL;
    context.flags = ARGON2_DEFAULT_FLAGS;
    context.version = ARGON2_VERSION_13;
    context.pwd = CONST_CAST(uint8_t *)pwd;

    //sprintf(benchDebugFilename, "argon2-bench2-thread-%d.txt", pthread_self());
    //benchDebugOut = fopen(benchDebugFilename, "w");

    while (1) {

        start, end = 0;


#ifdef WIN32
        WaitForSingleObject(lock, INFINITE);
#else
        pthread_mutex_lock(&lock);
#endif
        // Verify we have a workload to crunch & copy process-global dataset into thread-stack-local buffers
        if (g_length > 0 && g_end > 0 && g_start < g_end){

            // Snag some work for this thread
            start = g_start;

            end = g_start + g_batch;
            if (end > g_end) {
                end = g_end;
            }

            g_start = end;  // Then advance g_start so another thread can snag the next batch
            g_working++;

            length = (int32_t) g_length; // Size of block being mined (including transactions)
            for (i = 0; i < length; i++)
                pwd[i] = g_pwd[i];

            pwd[length+4] = 0;
            pwd[length+5] = 0;

            // Difficulty; each byte of the resulting 32-byte hash must be LESS THAN the specified value
            // in the g_difficulty[] array of the same byte-position.
            for (i=0; i<32; i++)
                target[i] = g_difficulty[i];

            for (i=0; i<32; i++)
                bestHash[i] = 255;

            context.pwdlen = (uint32_t)(length+4);

        }
        idPrev = g_id;  // Set idPrev so we can check later if we're working on a stale dataset
#ifdef WIN32
        ReleaseMutex(lock);
#else
        pthread_mutex_unlock(&lock);
#endif

        // should only trigger if (g_length > 0 && g_end > 0 && g_start < g_end) evaluated false
        if ( end == 0 ) {
#ifdef WIN32
            Sleep(2);
#else
            usleep(2);
#endif
            continue;
        }
        //bench_debug("Starting work: start: %lu; end: %lu\n", start, end);
        //bench_flush();


        // Perform argon2d hash search of current dataset

        solution = 0;

        for (j = start; j < end && solution == 0; ++j) {

            // Nonce is appended to block in Big-Endian format
            pwd[length + 3] = j & 0xff ;
            pwd[length + 2] = j >> 8 & 0xff ;
            pwd[length + 1] = j >> 16 & 0xff ;
            pwd[length    ] = j >> 24 & 0xff ;

            result = argon2_ctx(&context, type);

            if (result != ARGON2_OK) {
                printf("Error Argon2d");
                return 0;
            }

            for (i=0; i < 32; ++i){
                if (  bestHash[i] ==  out[i] ) continue; else
                if (  bestHash[i] <  out[i] ) break; else
                if (  bestHash[i] >  out[i] ){

                    for (i=0; i < 32; i++)
                        bestHash[i] = out[i];

                    bestHashNonce  = j;

                    for (i=0; i< 32; ++i){
                        if (  target[i] ==  bestHash[i] ) continue; else
                        if (  target[i] <  bestHash[i] ) break; else
                        if (  target[i] >  bestHash[i] ){
                            // Found a winner!!!

#ifdef WIN32
                            WaitForSingleObject(lock, INFINITE);
#else
                            pthread_mutex_lock(&lock);
#endif

                            if (idPrev == g_id) {  // check we're still working on the latest dataset
                                g_start = g_end+1; // Make sure no threads try working on more of this dataset
                                solution = 1;
                            } else {     // the dataset changed already, so this result is stale.
                                j = end; // force the outer for() loop to terminate
                            }

#ifdef WIN32
                            ReleaseMutex(lock);
#else
                            pthread_mutex_unlock(&lock);
#endif
                            //if (solution == 1) {
                            //    bench_debug("Found a solution; nonce=%lu\n", bestHashNonce);
                            //    bench_flush();
                            //}
                            break;

                        }
                    }

                    break;

                }
            }


        }


        if (g_debug)
            printf("processing E111NDED %lu %lu initially %lu %lu \n", start, end, g_start, g_end );

#ifdef WIN32
        WaitForSingleObject(lock, INFINITE);
#else
        pthread_mutex_lock(&lock);
#endif

        g_working--;

        if (g_id == idPrev){  // Recent mining is still relevant to the active dataset


            g_hashesTotal += (j-start);
            g_workersUsed++;


            for (i=0; i< 32; ++i){
                if (  g_bestHash[i] ==  bestHash[i] ) continue; else
                if (  g_bestHash[i] <  bestHash[i] ) break; else
                if (  g_bestHash[i] >  bestHash[i] ){

                    for (j=0; j < 32; j++)
                        g_bestHash[j] = bestHash[j];

                    g_bestHashNonce  = bestHashNonce;

                    break;
                }
            }

            if (g_debug) {
                printf("processing ENDED %lu %lu initially %lu %lu \n", start, end, g_start, g_end);
                printf("g_working ENDED %d \n", g_working);
            }


            if ( (solution == 1) || (g_working == 0 && g_start == g_end  && g_end != 0)){

                for (i=0; i < 32; i++){

                    hash[2*i]   = arr[ (int) g_bestHash[i]/16 ];
                    hash[2*i+1] = arr[ (int) g_bestHash[i]%16 ];

                }

                hash[64] = 0;


                double elapsed = 10;

                if (g_debug) {
                    printf("Time elapsed in s: %f \n", elapsed/g_workersUsed/1000);
                    printf("H/s: %f \n",( g_hashesTotal/elapsed*g_workersUsed*1000));
                    tstop = clock();
                    elapsed = (double)(tstop - g_tstart) * 1000.0 / CLOCKS_PER_SEC;
                }

                fout = fopen(g_filenameOutput, "w");
                if (!fout) {
                    // Trouble writing output!
                    break;
                }
                if (solution == 1)
                    fprintf(fout, "{ \"type\": \"s\", \"hash\": \"%s\", \"nonce\": %lu , \"h\": %lu }", hash, g_bestHashNonce, (unsigned long) (0));
                else
                    // What is the point of this?  Just a heartbeat to inform the miner pool that we are still mining so we get credit for attempting?
                    fprintf(fout, "{ \"type\": \"b\", \"bestHash\": \"%s\", \"bestNonce\": %lu , \"h\": %lu }", hash, g_bestHashNonce, (unsigned long) (0));

                fclose(fout);

            }


        } else {
            // idPrev != g_id, the dataset changed and we just need to loop again
        }

#ifdef WIN32
        ReleaseMutex(lock);
#else
        pthread_mutex_unlock(&lock);
#endif

    }



}


int main(int argc, char **argv ) {


    int i, err, ok;

    argon2_select_impl(stderr, "[libargon2] ");

    printf("PARSING \n");
    /* parse options */
    for (i = 0; i < argc; i++) {
        const char *a = argv[i];

        if (!strcmp(a, "-f")) {
            if (i < argc - 1) {
                i++;
                g_filename = argv[i];
                strcpy(g_filenameOutput, g_filename);
                strcat(g_filenameOutput, "output");
                continue;
            } else {
                printf("missing -f argument");
                return 0;
            }
        } else
        if (!strcmp(a, "-d")) {
            if (i < argc - 1) {
                i++;
                g_debug = atoi(argv[i]);;
                continue;
            } else {
                printf("missing -f argument");
                return 0;
            }
        } else if (!strcmp(a, "-b")) {
            if (i < argc - 1) {
                i++;
                g_batch = atoi(argv[i]);
                continue;
            } else {
                printf("missing -m argument");
                return 0;
            }
        } else if (!strcmp(a, "-c")) {
            if (i < argc - 1) {
                i++;
                g_cores = atoi(argv[i]);
                continue;
            } else {
                printf("missing -c argument");
                return 0;
            }
        }
    }

    printf("Argv was read \n");
    printf("cores %d \n", g_cores);
    printf("batch %d \n", g_batch);


#ifdef WIN32
     lock = CreateMutex( NULL, FALSE, NULL);
     lockOutput = CreateMutex( NULL, FALSE, NULL);
     if (lock == NULL) {
        printf("\n mutex init has failed\n");
        return -1;
     }
     if (lockOutput == NULL) {
        printf("\n mutex init has failed\n");
        return -1;
     }
#else
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return -1;
    }
    if (pthread_mutex_init(&lockOutput, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return -1;
    }

#endif


    for (i=0; i < g_cores; i++){

#ifdef WIN32

        HANDLE thread = CreateThread(NULL, 0, benchmark, NULL, 0, NULL);
        tid[i] = thread;

        Sleep(1);
#else

        err = pthread_create(&(tid[i]), NULL, &benchmark, NULL);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread created successfully\n");

        usleep(1);

#endif

    }

    //sprintf(rdDebugFilename, "argon2-bench2-readdata-%d.txt", getpid());
    //rdDebugOut = fopen(rdDebugFilename, "a");
    //if (!rdDebugOut) {
    //    exit(1);
    //}


    while (  1 ){

        if ( readData(g_filename) == -1 )
            break;

#ifdef WIN32
        Sleep(10);
#else
        usleep(10);
#endif

    }

    printf("Threads had been terminated \n");

#ifdef WIN32
    Sleep(200);
    ReleaseMutex(lock);
    ReleaseMutex(lockOutput);
    Sleep(200);
    printf("WaitForMultipleObjects return: %d error: %d\n",
         (DWORD)WaitForMultipleObjects(g_cores, tid, TRUE, INFINITE), GetLastError());

#else
    usleep(200);
    for (i=0; i < g_cores; i++)
        pthread_cancel(tid[i]);

    usleep(200);

    for (i=0; i < g_cores; i++)
        pthread_exit( i );

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lockOutput);
#endif

    return ARGON2_OK;
}
