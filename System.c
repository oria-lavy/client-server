#include "System.h"
#include "queue.h"
#include <stdlib.h>


System systemCreate(Algorithm alg, int request_max_num, int threads_num){
    if(alg != BLOCK && alg != DROP_HEAD && alg != DROP_TAIL && alg != DROP_RANDOM){
        return NULL;
    }
    if(request_max_num <= 0){
        return NULL;
    }
    System new_system = (System)malloc(sizeof(*new_system));
    if(new_system == NULL){
        return NULL;
    }
    int w_q_size;
    // if(request_max_num - threads_num > 0){
    //     w_q_size = request_max_num - threads_num;
    // }
    // else{
    //     w_q_size = 0;
    // }
    new_system->waiting_queue = create_queue();
    if(new_system->waiting_queue == NULL){
        free(new_system);
        return NULL;
    }
    new_system->running_queue = create_queue();
    if(new_system->running_queue == NULL){
        free(new_system->waiting_queue);
        free(new_system);
        return NULL;
    }
    new_system->alg = alg;
    new_system->request_max_num = request_max_num;
    new_system->threads_num = threads_num;
    return new_system;
}

int getMaxNumRequest(System system){
    return system->request_max_num;
}

Algorithm getAlg(System system){
    return system->alg;
}

bool isSystemFull(System system){
    int w_req = get_queue_size(system->waiting_queue);
    int r_req = get_queue_size(system->running_queue);
    if(w_req + r_req >= system->request_max_num){
        return true;
    }
    return false;
}

void addRequest(System system, int conn_fd, struct timeval arrival, struct timeval dispatch)
{
    if (system == NULL)
    {
        return;
        //exit(1);
    }
    QUEUE queue_to_enter;
    if (dispatch.tv_sec == 0 && dispatch.tv_usec == 0) //didnt start to run yet
    {
        queue_to_enter = system->waiting_queue;
    }
    else
    {
        queue_to_enter = system->running_queue;
    }
    add_to_queue(queue_to_enter, conn_fd, arrival, dispatch);
}

int runningQueueSize(System system){
    return get_queue_size(system->running_queue);
}

int waitingQueueSize(System system){
    return get_queue_size(system->waiting_queue);
}

bool isSystemEmpty(System system){
    if((waitingQueueSize(system) + runningQueueSize(system)) == 0){
        return true;
    }
    return false;
}

void destroySystem(System system){
    if(system == NULL){
        return;
    }
    destroy_queue(system->waiting_queue);
    destroy_queue(system->running_queue);
    free(system->waiting_queue);
    free(system->running_queue);
    free(system);
    return;
}