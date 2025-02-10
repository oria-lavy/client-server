#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

QUEUE_NODE create_node(int fd, struct timeval arrival, struct timeval dispatch)
{
    QUEUE_NODE node = (QUEUE_NODE)malloc(sizeof(*node));
    if (node == NULL)
    {
        return NULL;
    }
    node->process_fd = fd;
    node->arrival_time = arrival;
    node->dispatch_interval = dispatch;
    node->next = NULL;
    //check type
    return node;
}

QUEUE create_queue()
{
    struct timeval init;
    init.tv_sec = 0;
    init.tv_usec = 0;
    QUEUE_NODE dummy_node = create_node(-1, init, init);
    QUEUE queue = (QUEUE)malloc(sizeof(*queue));
    if (queue == NULL)
    {
        return NULL;
    }
    queue->head = dummy_node;
    //queue->last = NULL;
    queue->queue_size=0;
    return queue;
}

bool is_same_node(int first_fd, int second_fd)
{
    return first_fd == second_fd; 
}

int get_first_node_fd(QUEUE queue)
{
    if (is_queue_empty(queue))
    {
        return -1;
    }
    return queue->head->next->process_fd;
}

void add_to_queue(QUEUE queue, int fd, struct timeval arrival, struct timeval dispatch)
{
    if (queue == NULL)
    {
        return;
    }
    QUEUE_NODE node_to_add = create_node(fd, arrival, dispatch);
    QUEUE_NODE current_node = queue->head;
    while (current_node->next != NULL && !is_same_node(current_node->process_fd, fd))
    {
        current_node = current_node->next;
    }
    if (is_same_node(current_node->process_fd, fd)) //node already exists
    {
        return;
    }
    current_node->next = node_to_add;
    queue->queue_size++;
    return;
}
void printQueue(QUEUE queue)
{
    QUEUE_NODE q=queue->head;
    while(q->next!=NULL)
    {
        q=q->next;
        printf(" %d-->\n",q->process_fd);
    }
}
int get_queue_size(QUEUE queue)
{
    if (queue == NULL)
    {
        return -1;;
    }
    return queue->queue_size;
}

bool is_queue_empty(QUEUE queue)
{
    if (queue == NULL)
    {
        return 0;
    }
    return queue->queue_size == 0;
}

QUEUE_NODE get_node_by_fd(QUEUE queue, int fd)
{
    if (queue == NULL)
    {
        return NULL;
    }
    QUEUE_NODE current_node = queue->head;
    while(current_node->process_fd != fd && current_node->next != NULL)
    {
        current_node = current_node->next;
    }
    if (current_node->process_fd == fd)
    {
        return current_node;
    }
    else //node with this fd doesnt exist
    {
        return NULL;
    }
}

void remove_from_queue(QUEUE queue, int fd)
{
    if (queue == NULL)
    {
        return;
    }
    QUEUE_NODE current_node = queue->head;
    QUEUE_NODE prev_node = NULL;
    while (!is_same_node(current_node->process_fd, fd) && current_node->next != NULL)//current_node->next
    {
        prev_node = current_node;
        current_node = current_node->next;
    }
    if (is_same_node(current_node->process_fd, fd))
    {
        prev_node->next = current_node->next;
        queue->queue_size--;
        free(current_node);
    }
    return;

}

struct timeval get_arrival_by_fd(QUEUE queue, int fd)
{
    QUEUE_NODE node = get_node_by_fd(queue, fd);
    if (node == NULL)
    {
        struct timeval init;
        init.tv_sec = 0;
        init.tv_usec = 0;
        return init;
    }
    return node->arrival_time;
}

struct timeval get_dispatch_by_fd(QUEUE queue, int fd)
{

    QUEUE_NODE node = get_node_by_fd(queue, fd);
    if (node == NULL)
    {
        struct timeval init;
        init.tv_sec = 0;
        init.tv_usec = 0;
        return init;
    }
    return node->dispatch_interval;
}

QUEUE_NODE get_node_by_index (QUEUE queue, int index)
{
    if (queue == NULL || index > queue->queue_size)
    {
        return NULL;
    }
    QUEUE_NODE current = queue->head;
    for (int i=0; i < index; i++)
    {
        if (current->next == NULL)
        {
            return NULL;
        }
        current = current->next;
    }
    return current;
}

void clear_queue(QUEUE queue)
{
    if (queue == NULL)
    {
        return;
    }
    QUEUE_NODE tmp = queue->head;
    while(queue->head != NULL){
        tmp = queue->head;
        queue->head = queue->head->next;
        if(queue->head == NULL){
            free(tmp);
            break;
        }
        remove_from_queue(queue, tmp->process_fd);
    }

    queue->queue_size = 0;
    return;
}

void destroy_queue(QUEUE queue)
{
    clear_queue(queue);
    //free(queue);
    return;
}








