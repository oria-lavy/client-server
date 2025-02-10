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
        return; //check if needs something else
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
 
        int first_fd = get_first_node_fd(system->waiting_queue); //I think shouldnt be null because we call consumer if the queue isnt empty
        struct timeval dispatch;
        gettimeofday(&dispatch, NULL);
        struct timeval arrival = get_arrival_by_fd(system->waiting_queue, first_fd);
        remove_from_queue(system->waiting_queue, first_fd); //remove node from waiting queue.

        add_to_queue(system->running_queue, first_fd, arrival, dispatch); //add to running queue.

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
        client_len = sizeof(client_addr);
        conn_fd = accept(listen_fd, (SA*)&client_addr, (socklen_t*)&client_len);
        gettimeofday(&arrival_time, NULL);
        pthread_mutex_lock(&mutex_lock);
        if (isSystemFull(system)) {
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
            addRequest(system, conn_fd, arrival_time, init); //waiting q
        }
        pthread_cond_signal(&consumer_allowed);
        pthread_mutex_unlock(&mutex_lock);

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


    ///create threads

    threads_array = malloc(sizeof(Thread_struct) * threads_num); //global variable

    if(threads_array == NULL) {
        return -1;
    }
    for (i = 0; i < threads_num; i++) {
        threads_array[i].stats=(Stats)malloc(sizeof(struct stats_t));
	    struct thread_arguments args;
	    args.system=system;
        args.index=i;
        if(pthread_create(&threads_array[i].thread, NULL, &consumer, &args) != 0) {

        }

    }
    producer(system, port);
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


    


 
