#include "queue.h"

int topo = -1, fundo = -1, fila[SIZE_QUEUE] = {SIZE_QUEUE + 1};

int isFull() {
    if ((topo == fundo + 1) || (topo == 0 && fundo == SIZE_QUEUE - 1)){
        return 1;
    }
    return 0;
}

int isEmpty() {
    if (topo == -1) return 1;
    return 0;
}

void enQueue(int team) {
    if(isFull()){
        return;
    }
    if (topo == -1){ 
        topo = 0;
    }
    fundo = (fundo + 1) % SIZE_QUEUE;
    fila[fundo] = team;
}

int deQueue() {
    int team;
    if (isEmpty()) {
        return (-1);
    } else {
        team = fila[topo];
        fila[topo] = SIZE_QUEUE + 1;
        if (topo == fundo) {
            topo = -1;
            fundo = -1;
        } 
        else {
            topo = (topo + 1) % SIZE_QUEUE;
        }
        return (team);
    }
}

int checkQueue(int team){
    if(isEmpty()){
        return 1;
    }
    if(topo == 0 && fundo == 0){                                //Caso topo e fundo serem 0s e o time ja foi inserido
        if(fila[0] == team){
            return 0;
        }
    }
    for (int i = topo; i != fundo; i = (i+1) % SIZE_QUEUE) {
        if(fila[i] == team || fila[fundo] == team){
            return 0;
        }
    }
    return 1;
}

// Mostra a fila (por enquanto apenas no USART)
void display() {
    if (isEmpty())
        EnviaStr_USART("Fila vazia\n");
    else {
        for (int i = topo; i != fundo; i = (i+1) % SIZE_QUEUE) {
            EnviaNum_USART(fila[i]);
        }
        EnviaNum_USART(fila[fundo]);
    }
}

