#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Queue.h"

void createQueue(Queue *queue) {
    queue->front = -1;
    queue->rear = -1;
    queue->size = 0;
    queue->current = 0;
}

int isFull(Queue *queue) {
    if ((queue->front == queue->rear + 1) || (queue->front == 0 && queue->rear == SIZE_QUEUE - 1)) {
        return 1;
    }
    return 0;
}

int isEmpty(Queue *queue) {
    if (queue->front == -1) return 1;
    else return 0;
}

void enQueue(Queue *queue, int team) {
    if (!isFull(queue)) {
        if (queue->front == -1) { 
            queue->front = 0;
        }
        queue->rear = (queue->rear + 1) % SIZE_QUEUE;
        queue->elements[queue->rear] = team;
        queue->size++;
    }
}

int deQueue(Queue *queue) {
    if (isEmpty(queue)) {
        queue->current = 0;
        return (-1);
    } else {
        queue->size--;
        queue->current = queue->elements[queue->front];
        queue->elements[queue->front] = SIZE_QUEUE + 1;
        if (queue->front == queue->rear) {
            queue->front = -1;
            queue->rear = -1;
        } 
        else {
            queue->front = (queue->front + 1) % SIZE_QUEUE;
        }
        return (queue->current);
    }
}

int checkQueue(Queue *queue, int team) {
    if(isEmpty(queue)) {
        return 1;
    }
    if (queue->front == 0 && queue->rear == 0) {
        if (queue->elements[0] == team) {
            return 0;
        }
    }
    for (int i = queue->front; i != queue->rear; i = (i+1) % SIZE_QUEUE) {
        if (queue->elements[i] == team || queue->elements[queue->rear] == team) {
            return 0;
        }
    }
    return 1;
}

// Retorna os dados da fila no formado data:fila;time sendo atendido;tamaho da fila
void get_queue_data(Queue *queue, char queue_data_buffer[50]) {
    char aux[50];
    memset(queue_data_buffer, 0, 50);
    strcpy(queue_data_buffer, "data:");

    int i = queue->front, j = strlen(queue_data_buffer);
    for (; i != queue->rear; i = (i+1) % SIZE_QUEUE, j++) {
        if (queue->elements[i]) {                                       // Para nao enviar o time -1
            queue_data_buffer[j] = queue->elements[i] - 1  + 48;
        }
    }

   if (queue->elements[queue->rear]) {                                  // Para nao enviar o time -1
        queue_data_buffer[j++] = queue->elements[queue->rear] - 1  + 48;
    }
    queue_data_buffer[j++] = ';';

    queue_data_buffer[j++] = queue->current - 1  + 48;
    queue_data_buffer[j++] = ';';

    sprintf(aux, "%d;", queue->size);
    strcat(queue_data_buffer, aux);
}