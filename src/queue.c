#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

int topo = -1, fundo = -1, fila[SIZE_QUEUE] = {SIZE_QUEUE + 1}, time_atual, qtd_times;

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
    qtd_times++;
}

int deQueue() {
    if (isEmpty()) {
        return (-1);
    } else {
        qtd_times--;
        time_atual = fila[topo];
        fila[topo] = SIZE_QUEUE + 1;
        if (topo == fundo) {
            topo = -1;
            fundo = -1;
        } 
        else {
            topo = (topo + 1) % SIZE_QUEUE;
        }
        return (time_atual);
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
    char mostra_fila[11] = {};
    char aux[5];
    int i = topo, j = 0;

    if(isEmpty()){
        return;
    }

    for (; i != fundo && j < 4; i = (i+1) % SIZE_QUEUE, j++) {
       mostra_fila[j] = fila[i] - 1  + 48;
    }

    mostra_fila[j++] = fila[fundo] - 1  + 48;
    mostra_fila[j++] = ';';                                       //Para separar as categorias times na fila; 

    mostra_fila[j++] = time_atual - 1  + 48;
    mostra_fila[j++] = ';';                                       //time sendo atendido;

    sprintf(aux, "%d", qtd_times);
    strcat(mostra_fila, aux);

    EnviaStr_USART(mostra_fila);
    
}