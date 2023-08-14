#ifndef WEBSERVER_FILES_SYSTEM_H
#define WEBSERVER_FILES_SYSTEM_H

#include <stdbool.h>
#include "queue.h"
#include <pthread.h>

typedef enum{
    BLOCK,
    DROP_TAIL,
    DROP_HEAD,
    DROP_RANDOM
}Algorithm;

typedef struct stats_t
{
    int index;
    int total_req_cnt;
    int static_cnt;
    int dynamic_cnt;
} *Stats;

struct System_t {
    QUEUE waiting_queue;
    QUEUE running_queue;
    int request_max_num;
    int request_cnt;
    int threads_num;
    Algorithm alg;
};

typedef struct System_t *System;

typedef struct thread_st{
    pthread_t thread;
    Stats stats;
    int index;
} Thread_struct;

System systemCreate(Algorithm alg, int request_max_num, int threads_num);
int getMaxNumRequest(System system);
Algorithm getAlg(System system);
bool isSystemFull(System system);
int runningQueueSize(System system);
int waitingQueueSize(System system);
bool isSystemEmpty(System system);
void addRequest(System system, int conn_fd, struct timeval arrival, struct timeval dispatch); /// to implement
void destroySystem(System system);

#endif //WEBSERVER_FILES_SYSTEM_H
