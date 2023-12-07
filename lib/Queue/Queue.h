#ifndef QUEUE_H_   
#define QUEUE_H_

#define SIZE_QUEUE 11

//Funcoes da fila circular, baseado em https://www.programiz.com/dsa/circular-queue

typedef struct Queue {
    int elements[SIZE_QUEUE];
    int front;
    int rear;
    int size;
    int current;
} Queue;

void createQueue(Queue *queue) ;
int checkQueue(Queue *queue, int team);
int deQueue(Queue *queue);
void enQueue(Queue *queue, int team);
int isFull(Queue *queue);
int isEmpty(Queue *queue);
void get_queue_data(Queue *queue, char destiny[50]);

#endif // QUEUE_H_
