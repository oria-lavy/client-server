#include "segel.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

/** ____DECLARATIONS_____ */
pthread_mutex_t mutex_lock;
pthread_cond_t consumer_allowed;
pthread_cond_t producer_allowed;
Thread_struct* threads_array;
int i=0;
int threads_num;
int requests_max_num;
Algorithm alg;

/** ALGORITHM FUNCTIONS **/
void blockAlg(System system)
{
    // if (system == NULL)
    // {
    //     return;
    //     //exit(1);
    // }
    while(isSystemFull(system))
    {
        pthread_cond_wait(&producer_allowed, &mutex_lock);
    }
}

void dropHeadAlg(System system)
{
    if (system == NULL)
    {
        return;
        //exit(1);
    }
    int fd = get_first_node_fd(system->waiting_queue);
    remove_from_queue(system->waiting_queue, fd);

    Close(fd); //we get rid of this request so we close its fd
}

void dropRandAlg(System system)
{
    int size=system->waiting_queue->queue_size;

    int drop_count= size/2 +size %2;
    while(drop_count>0) {
        int index =( rand() % size )+ 1;
        QUEUE_NODE node=get_node_by_index(system->waiting_queue, index);
        Close(node->process_fd);
        remove_from_queue(system->waiting_queue,node->process_fd);
        size=system->waiting_queue->queue_size;
        drop_count--;
    }
}



// our variable will be the num of requests
// that are allowed in the queue - systemFull() func
struct thread_arguments{
System system;
int index;
};
// HW3: Parse the new arguments too
void getargs(int *port, int* threads_num, int* max_num_request, Algorithm* alg, int argc, char *argv[])
{
    if (argc < 5) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	return;
    //exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *max_num_request = atoi(argv[3]);
    char* sched_alg = argv[4];
    if(strcmp(sched_alg, "block") == 0) {
        *alg = BLOCK;
    }
    else if (strcmp(sched_alg, "dt") == 0) {
        *alg = DROP_TAIL;
    }
    else if (strcmp(sched_alg, "dh") == 0) {
        *alg = DROP_HEAD;
    }
    else if (strcmp(sched_alg, "random") == 0) {
        *alg = DROP_RANDOM;
    }
    else {
        return;
        //exit(1); //check if needs something else
    }
}

void* consumer(void* arg){ //while1 etzel tomguy
    if(arg == NULL){
        return NULL;
    }
    struct thread_arguments arguments = *((struct thread_arguments*)arg); ///we dont have = operator (?)
    Stats stats = (Stats)malloc(sizeof(stats));
    System system=arguments.system;
    int index=arguments.index;
    stats->dynamic_cnt = 0;
    stats->static_cnt = 0;
    stats->total_req_cnt = 0;
    while(1) {
        pthread_mutex_lock(&mutex_lock);
        while (system->waiting_queue->queue_size==0) { //there are no requests waiting
            pthread_cond_wait(&consumer_allowed, &mutex_lock);
        }
        // struct timeval time;
        // if (gettimeofday(&time, NULL) == -1) //0 on success and -1 on failure
        // {
        //     return NULL;
        //     //exit(1);
        // }
        int first_fd = get_first_node_fd(system->waiting_queue); //I think shouldnt be null because we call consumer if the queue isnt empty
        struct timeval dispatch;
        gettimeofday(&dispatch, NULL);
        //int first_fd = first_node->process_fd;
        struct timeval arrival = get_arrival_by_fd(system->waiting_queue, first_fd);
        remove_from_queue(system->waiting_queue, first_fd); //remove node from waiting queue.

        add_to_queue(system->running_queue, first_fd, arrival, dispatch); //add to running queue.
        /*int index = 0;
        pthread_t wanted_id = pthread_self();
        while(index<threads_num){
            if(threads_array[index].thread == wanted_id)
                break;
            index++;
        }*/
        pthread_mutex_unlock(&mutex_lock);


        requestHandle(first_fd, index, &stats, arrival, dispatch);

        pthread_mutex_lock(&mutex_lock);
        remove_from_queue(system->running_queue, first_fd);
        pthread_cond_signal(&producer_allowed);
        Close(first_fd);
        pthread_mutex_unlock(&mutex_lock);

    }
    return NULL;
}

void* producer(System system, int port) {

    int listen_fd = Open_listenfd(port);
    //printf("we are at producer, line 140 \n");
    int client_len;
    int conn_fd;
    struct sockaddr_in client_addr;
    struct timeval arrival_time;
    Algorithm alg = getAlg(system);
    struct timeval init;
    init.tv_sec = 0;
    init.tv_usec = 0;
    //printf("init \n");

    while(1) {
        //printf("im in while \n");
        client_len = sizeof(client_addr);
        //printf("line 154, while \n");
        conn_fd = accept(listen_fd, (SA*)&client_addr, (socklen_t*)&client_len);
        //printf("after accept, while \n");
        gettimeofday(&arrival_time, NULL);
        //printf("line 156, while \n");
        pthread_mutex_lock(&mutex_lock);
        //printf("line 158, while \n");
        if (isSystemFull(system)) {
            //printf("im in if \n");
            if (alg == BLOCK) {
                blockAlg(system);
                addRequest(system, conn_fd, arrival_time, init); //waiting q
            } else if (alg == DROP_TAIL) {
                Close(conn_fd);
                pthread_mutex_unlock(&mutex_lock);
                continue;
            } else if (alg == DROP_HEAD) {
                if (system->waiting_queue->queue_size == 0) {
                    Close(conn_fd);
                    pthread_mutex_unlock(&mutex_lock);
                    continue;
                }
                dropHeadAlg(system);
                printQueue(system->waiting_queue);
               // printQueue(system->running_queue);
                addRequest(system, conn_fd, arrival_time, init); //waiting q

                
            } else if (alg == DROP_RANDOM) {
                if(system->waiting_queue->queue_size==0)
                {
                    Close(conn_fd);
                    pthread_mutex_unlock(&mutex_lock);
                    continue;
                }
                dropRandAlg(system);
                addRequest(system,conn_fd,arrival_time,init);
            }
        } else {
            //printf("line 176, while \n");
            addRequest(system, conn_fd, arrival_time, init); //waiting q
        }
        pthread_cond_signal(&consumer_allowed);
        pthread_mutex_unlock(&mutex_lock);
        
        //Close(listen_fd);
        //printf("end of while \n");
        //shouldnt we close listen_fd? haomnam?!!!!!!
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    


    getargs(&port, &threads_num, &requests_max_num, &alg, argc, argv);

    ///initializing mutex:
    pthread_mutex_init(&mutex_lock, NULL);
    pthread_cond_init(&consumer_allowed, NULL);
    pthread_cond_init(&producer_allowed, NULL);
    System system = systemCreate(alg, requests_max_num, threads_num);



    //System system = systemCreate(BLOCK, 2, 1);

//    struct timeval time;
//    gettimeofday(&time,0);

//    add_to_queue(system->waiting_queue, 1, time, time);
//    add_to_queue(system->waiting_queue, 2, time, time);
//    add_to_queue(system->running_queue, 3, time, time);
//    add_to_queue(system->running_queue, 8, time, time);
//    printf("first fd is %d \n", system->waiting_queue->head->next->process_fd);
//    printf("second fd is %d \n", system->waiting_queue->head->next->next->process_fd);
//    printf("third fd is %d \n", system->running_queue->head->next->process_fd);
//    printf("4 fd is %d \n", system->running_queue->head->next->next->process_fd);
//    remove_from_queue(system->waiting_queue, system->waiting_queue->head->next);
//    printf("after remove, firstw fd is %d \n",  system->waiting_queue->head->next->process_fd); //should be 2
//    remove_from_queue(system->running_queue, get_node_by_fd(system->running_queue, 8));
//    printf("after remove, firstr fd is %d \n", system->running_queue->head->next->process_fd);
//    printf("wsize is %d \n", system->waiting_queue->queue_size);
//    printf("rsize is %d \n", system->running_queue->queue_size);
//    add_to_queue(system->waiting_queue, 2022, time, time);
//    printf("wsize after first add of 2022 is %d \n", system->waiting_queue->queue_size);
//    add_to_queue(system->waiting_queue, 2022, time, time);
//    printf("wsize after second add of 2022 is %d \n", system->waiting_queue->queue_size);
//    add_to_queue(system->running_queue, 7654321, time, time);
//    printf("rsize after first add of 7654321 is %d \n", system->running_queue->queue_size);
//    clear_queue(system->waiting_queue);
//    clear_queue(system->running_queue);
//    printf("wsize is %d \n", system->waiting_queue->queue_size);
//    printf("rsize is %d \n", system->running_queue->queue_size);
//    destroySystem(system);


    if(system == NULL) {
        return -1;
        //exit(1); ///check if needs perror
    }

    ///create threads

    threads_array = malloc(sizeof(Thread_struct) * threads_num); //global variable

    if(threads_array == NULL) {
        return -1;
        //perror("Failed to allocate\n");
        //exit(1);
    }
    for (i = 0; i < threads_num; i++) {
        threads_array[i].stats=(Stats)malloc(sizeof(struct stats_t));
	    struct thread_arguments args;
	    args.system=system;
	    //args.stats=(Stats)malloc(sizeof(struct stats_t));
        args.index=i;
        if(pthread_create(&threads_array[i].thread, NULL, &consumer, &args) != 0) {
            //destroySystem(system);
            //free(threads_array);
            //exit(1);
            //return -1;
        }
        //threads_array[i].index = i;
       // threads_array[i].stats->total_req_cnt = 0;
       // threads_array[i].stats->dynamic_cnt = 0;
        //threads_array[i].stats->static_cnt = 0;
    }
    producer(system, port);
///printf("line 223 after producer\n");
    for (int i=0; i < threads_num; i++) {
        if (pthread_join(threads_array[i].thread, NULL) != 0) {
            destroySystem(system);
            free(threads_array);
            //exit(1);
            return -1;
        }
    }

	//
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads
	// do the work.
	//

    pthread_mutex_destroy(&mutex_lock);
    pthread_cond_destroy(&consumer_allowed);
    pthread_cond_destroy(&producer_allowed);
    destroySystem(system);
    free(threads_array);

    return 0;
}


    


 
