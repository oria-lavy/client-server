#ifndef QUEUE_H
#define QUEUE_H
#include <stdbool.h>
#include <sys/time.h>

/****ENUM FOR REQUEST****/
typedef enum
{
    STATIC_REQ,
    DYNAMIC_REQ
} REQ_TYPE;

/****QUEUE STRUCT****/
typedef struct queue_node
{
    int process_fd;
    struct timeval arrival_time;
    struct timeval dispatch_interval;
    REQ_TYPE type;
    struct queue_node* next;
} *QUEUE_NODE;

typedef struct queue
{
    QUEUE_NODE head;
    //QUEUE_NODE last;
    int queue_size;
} *QUEUE;


/****QUEUE FUNCTIONS****/
QUEUE create_queue(); //done
void add_to_queue(QUEUE queue, int fd, struct timeval arrival, struct timeval dispatch); //done
int get_queue_size(QUEUE queue); //done
bool is_queue_full(QUEUE queue); //if we know max num of requests then should check this.
bool is_queue_empty(QUEUE queue); //done
void remove_from_queue(QUEUE queue, int fd); //done
QUEUE_NODE create_node(int fd, struct timeval arrival, struct timeval dispatch); //done
bool is_same_node(int first_fd, int second_fd); //done
int get_first_node_fd(QUEUE queue); //done
QUEUE_NODE get_node_by_fd(QUEUE queue, int fd); //done
struct timeval get_arrival_by_fd(QUEUE queue, int fd); //done
struct timeval get_dispatch_by_fd(QUEUE queue, int fd); //done
QUEUE_NODE get_node_by_index (QUEUE queue, int index);
//add more functions...

void printQueue(QUEUE queue);
void clear_queue(QUEUE queue); //done
void destroy_queue(QUEUE queue); //done


#endif

