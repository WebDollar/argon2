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
#include<pthread.h>
#include<unistd.h>
pthread_t tid[2];


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


uint32_t t_cost = 2;
uint32_t m_cost = 256;
argon2_type type = 0; //argon2_type.Argon2_d;
uint32_t thread_n = 2;



/*
 * Benchmarks Argon2 with salt length 16, password length 16, t_cost 3,
   and different m_cost and threads
 */

#ifdef WIN32
DWORD WINAPI benchmark(void* data) {
#else
void * benchmark() {
#endif

    clock_t tstop;
    char change, solution;
    unsigned char out[32];
    FILE * fout;

    uint32_t length;
    unsigned char pwd[1024*1026];
    unsigned char target[33], bestHash[33];
    unsigned long i;

    unsigned long j;
    unsigned long start = 0;
    unsigned long end = 0;
    unsigned long bestHashNonce;

    while ( 1 == 1){

        sleep(2);

        start, end = 0;

        pthread_mutex_lock(&lock);

        if (g_length > 0 && g_end > 0 && g_start < g_end){
            start = g_start;
            end = g_start + g_batch;
            if (end > g_end)
                end = g_end;
            g_start = end;

            length = g_length;
            memset(pwd, 0, length);
            for (i=0; i<length; i++)
                pwd[i] = g_pwd[i];

            for (i=0; i<32; i++)
                target[i] = g_difficulty[i];

            g_working++;
        }
        pthread_mutex_unlock(&lock);

        if ( end == 0)
            continue;

        for (i=0; i<32; i++)
            bestHash[i] = 255;

        solution = 0;

        printf("processing %lu %lu \n", start, end );
        for (j = start; j < end && solution == 0; ++j) {


            pwd[length + 3] = j & 0xff ;
            pwd[length + 2] = j >> 8 & 0xff ;
            pwd[length + 1] = j >> 16 & 0xff ;
            pwd[length    ] = j >> 24 & 0xff ;

            argon2_hash(t_cost, m_cost, thread_n, pwd, length+4,
                        "Satoshi_is_Finney", 17, out, 32, NULL, 0, type, ARGON2_VERSION_13);

            change = 0;
            for (i=0; i < 32; ++i){
                if (  bestHash[i] ==  out[i] ) continue; else
                if (  bestHash[i] <  out[i] ) break; else
                if (  bestHash[i] >  out[i] ){

                    change = 1;
                    break;

                }
            }


            if (  change == 1){

                for (i=0; i < 32; i++)
                    bestHash[i] = out[i];

                bestHashNonce  = j;

                for (i=0; i< 32; ++i){
                    if (  target[i] ==  bestHash[i] ) continue; else
                    if (  target[i] <  bestHash[i] ) break; else
                    if (  target[i] >  bestHash[i] ){


                        pthread_mutex_lock(&lock);
                        g_start = g_end;
                        pthread_mutex_unlock(&lock);


                        solution = 1;
                        break;

                    }
                }


            }


        }


        pthread_mutex_lock(&lock);

        g_hashesTotal += j-start;

        for (i=0; i< 32; ++i){
            if (  g_bestHash[i] ==  bestHash[i] ) continue; else
            if (  g_bestHash[i] <  bestHash[i] ) break; else
            if (  g_bestHash[i] >  bestHash[i] ){

                for (j=0; j < 32; j++) {
                    g_bestHash[j] = bestHash[j];
                }

                g_bestHashNonce  = bestHashNonce;

                break;

            }
        }

        g_working--;

        printf("processing ENDED %lu %lu initially %lu %lu \n", start, end, g_start, g_end );
        printf("g_working ENDED %d \n", g_working );

        if ( (solution == 1) || (g_working == 0 && start < end && g_end != 0)){


            printf("  Writing data \n");

            unsigned char hash[65];
            for (i=0; i < 32; i++){

                hash[2*i] = arr[ g_bestHash[i]/16 ];
                hash[2*i+1] = arr[ g_bestHash[i]%16 ];

            }

            tstop = clock();
            double elapsed = (double)(tstop - g_tstart) * 1000.0 / CLOCKS_PER_SEC;

            printf("Time elapsed in s: %f \n", elapsed/1000);
            printf("H/s: %lu \n",(unsigned long) ( g_hashesTotal*elapsed));

            fout = fopen(g_filenameOutput, "w");
            if (solution == 1)
                fprintf(fout, "{ \"type\": \"s\", \"hash\": \"%s\", \"nonce\": %lu , \"h\": %lu }", hash, g_bestHashNonce, (unsigned long) (g_hashesTotal*elapsed));
            else
                fprintf(fout, "{ \"type\": \"b\", \"bestHash\": \"%s\", \"bestNonce\": %lu , \"h\": %lu }", hash, g_bestHashNonce, (unsigned long) (g_hashesTotal*elapsed));

            fclose(fout);


        }
        pthread_mutex_unlock(&lock);

        //printf("H/s: %f \n", (end-start)/(elapsed/1000));


    }



}


int main(int argc, char **argv ) {


    int i, err, cores = 4;
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
                cores = atoi(argv[i]);
                continue;
            } else {
                printf("missing -c argument");
                return 0;
            }
        }
    }

    printf("Argv was read \n");
    printf("cores %d \n", cores);
    printf("batch %d \n", g_batch);


    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lockOutput, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    for (i=0; i < cores; i++){

        #ifdef WIN32

            HANDLE thread = CreateThread(NULL, 0, benchmark, NULL, 0, NULL);
            tid[i] = thread;

        #else

            err = pthread_create(&(tid[i]), NULL, &benchmark, NULL);
            if (err != 0)
                printf("\ncan't create thread :[%s]", strerror(err));
            else
                printf("\n Thread created successfully\n");


        #endif


        i++;
    }

    while (1 == 1){

        readData(g_filename);
        sleep(10);

    }

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lockOutput);

    return ARGON2_OK;
}