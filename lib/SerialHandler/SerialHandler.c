#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "stm32f1xx.h"

char tx_str[255];
unsigned int itx = 0;

void EnviaStr_USART(char *string) {
    while(*string){
        while (!(USART1->SR & USART_SR_TXE));       // Aguarda reg. de dado Tx estar vazio
        USART1->DR = *string;
        string++; 
    }
}

void EnviaNum_USART(int value) { 
    char int_str[100];
    sprintf(int_str, "%d", value);
    EnviaStr_USART(int_str);
}

void EnviaCod_USART(int code[200]) {
    EnviaStr_USART("Codigo: ");
    for (int i = 0; code[i] != 0; i++) {
        EnviaNum_USART(code[i]);
        EnviaStr_USART(" ");
    }
    EnviaStr_USART("\n");
}
