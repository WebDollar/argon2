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

pthread_mutex_t lock;
pthread_mutex_t lockOutput;

/*
 * Benchmarks Argon2 with salt length 16, password length 16, t_cost 3,
   and different m_cost and threads
 */

#ifdef WIN32
DWORD WINAPI benchmark(void* data) {
#else
void * benchmark() {
#endif

    unsigned char out[32];

    uint32_t length;
    unsigned char pwd[1024*1025];
    unsigned char target[32];
    int i;

    unsigned long j;
    unsigned long start = 0;
    unsigned long end = 30000;

    while ( 1 == 1){

        sleep(2);

        pthread_mutex_lock(&lock);
        if (g_length > 0 && g_start < g_end){
            start = g_start;
            end = g_start + g_batch;
            if (end > g_end)
                end = g_end;
            g_start = end;

            length = g_length;
            memset(pwd, 0, length);
            for (i=0; i<length; i++)
                pwd[i] = g_pwd[i];
        }
        pthread_mutex_unlock(&lock);


        //double run_time = 0;
        clock_t tstart = clock();

        for (j = start; j < end; ++j) {

            pwd[length + 3] = j & 0xff ;
            pwd[length + 2] = j >> 8 & 0xff ;
            pwd[length + 1] = j >> 16 & 0xff ;
            pwd[length    ] = j >> 24 & 0xff ;

            argon2_hash(t_cost, m_cost, thread_n, pwd, length+4,
                        "Satoshi_is_Finney", 17, out, 32, NULL, 0, type, ARGON2_VERSION_13);

        }

        clock_t tstop = clock();
        double elapsed = (double)(tstop - tstart) * 1000.0 / CLOCKS_PER_SEC;
        printf("Time elapsed in s: %f \n", elapsed/1000);
        //printf("H/s: %f \n", (end-start)/(elapsed/1000));


    }



}


int main(int argc, char **argv ) {


    int i, err, cores;
    argon2_select_impl(stderr, "[libargon2] ");

    /* parse options */
    for (i = 0; i < argc; i++) {
        const char *a = argv[i];
        unsigned long input = 0;
        if (!strcmp(a, "-f")) {
            if (i < argc - 1) {
                i++;
                g_filename = argv[i+1];
                continue;
            } else {
                printf("missing -f argument");
                return 0;
            }
        } else if (!strcmp(a, "-b")) {
            if (i < argc - 1) {
                i++;
                g_batch = strtoul(argv[i+1], NULL, 10);
                continue;
            } else {
                printf("missing -m argument");
                return 0;
            }
        } else if (!strcmp(a, "-c")) {
            if (i < argc - 1) {
                i++;
                cores = strtoul(argv[i+1], NULL, 10);
                continue;
            } else {
                printf("missing -c argument");
                return 0;
            }
        }
    }


    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lockOutput, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    for (i=0; i < 8; i++){

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

    for (i=0; i < 8; i++)
        pthread_join(tid[i], NULL);

    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lockOutput);



    return ARGON2_OK;
}