#ifndef QUEUE_H_   
#define QUEUE_H_

#define SIZE_QUEUE 3

//Funcoes da fila circular, baseado em https://www.programiz.com/dsa/circular-queue

extern int fundo, topo, fila[SIZE_QUEUE];

int checkQueue(int team);
int deQueue();
void enQueue(int team);
int isFull();
int isEmpty();
void display();

#endif // QUEUE_H_
