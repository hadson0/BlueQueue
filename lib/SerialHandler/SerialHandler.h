#ifndef SERIALHANDLER_H_   
#define SERIALHANDLER_H_

void int2str(int valor);
void EnviaStr_USART(char *string);
void EnviaNum_USART(int valor);
void EnviaCod_USART(int code[200]);

#endif // SERIALHANDLER_H_
