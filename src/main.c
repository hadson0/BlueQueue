#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "Queue.h"
#include "SerialHandler.h"

#include "stm32f1xx.h"

#define STR_LENGTH 5
#define CODE_LENGTH 200

Queue queue;
bool learning_mode = false;
int sgn_duration, t_ini;
int antibounce_delay;
int sgn_idx, team_idx, codes[11][CODE_LENGTH], buffer[CODE_LENGTH];

bool isCodeEmpty(int code[CODE_LENGTH]);
void unregister();
void compareCodes();
void process_signal();
void TIM2_IRQHandler();
void EXTI1_IRQHandler();
void EXTI2_IRQHandler();
void SysTick_Handler();
void USART1_IRQHandler();

int main() {
    // Habilita clock do barramento APB2
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN  | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN; 
    GPIOC->CRH |= GPIO_CRH_MODE13_1;                        // Configura pino PC13 como saida open-drain 2 MHz

    // Configura pino PA1|PA2 como entrada e PA3 como saida                     
    GPIOA->CRL = (GPIOA->CRL & 0xFFFF000F) | (0x8U << (4 * 1)) |(0x8U << (4 * 2)) | (0x3U << (4 * 3));
    GPIOA->ODR |=  GPIO_ODR_ODR1 | GPIO_ODR_ODR2;                           // Habilitar pull-up no pino PA1

    // Configura o USART1 para Tx e Rx sem IRQ
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFFF0F) | 0x000000B0;    // PA9 como saida push-pull em funcao alt. (Tx da USART1) 0b1011=0xB
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;                   // Hab. clock para USART1
    USART1->BRR  = 8000000/9600;                            // Define baudrate = 9600 baud/s (APB2_clock = 8 MHz)     
    USART1->CR1 |= (USART_CR1_RE | USART_CR1_TE);           // Hab. RX e TX
    USART1->CR1 |= USART_CR1_RXNEIE;                        // Hab. interrupção por RXNE
    USART1->CR1 |= USART_CR1_UE;                            // Hab USART1
    NVIC->ISER[1] = (uint32_t)(1 << (USART1_IRQn-32)); // Hab. IRQ da USART1 na NVIC

    // Configura o PA1|PA2 com interrupcao no EXTI1|EXTI2
    AFIO->EXTICR[0] = AFIO_EXTICR1_EXTI1_PA | AFIO_EXTICR1_EXTI2_PA;                // Seleciona PA1 para EXTI1
    EXTI->FTSR = EXTI_FTSR_FT1 | EXTI_FTSR_FT2;                                     // Sensivel na rampa de descida
    EXTI->IMR = EXTI_IMR_IM1 | EXTI_IMR_IM2;                                        // Hab. mascara de interrup. do EXTI1
    NVIC->ISER[0] = (uint32_t)(1 << EXTI1_IRQn) | (uint32_t)(1 << EXTI2_IRQn);      // Hab. IRQ do EXTI1 na NVIC

    // Config. TIM2 com entrada de captura no canal 1 (PA0)
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;             // Habilita clock do TIM2 do bus APB1
    TIM2->ARR = 0xFFFF;                             // Registrador de auto-carregamento (1kHz -> periodo max. 1ms)
    TIM2->PSC = 15;                                 // Prescaler
    TIM2->CNT = 0;                                  // Zera o contador

    // Config. a entrada
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;                // Sem filtro na entrada, sem prescaler na entrada, TI1 como entrada
    TIM2->CCER |= TIM_CCER_CC1P;                    // Sensivel na borda de descida da entrada
    TIM2->CCER |= TIM_CCER_CC1E;                    // Habilita a captura no CH1
    TIM2->SR &= ~TIM_SR_CC1IF;                      // Apaga flag sinalizadora da IRQ
    TIM2->DIER |= TIM_DIER_UIE | TIM_DIER_CC1IE;    // Habilita IRQ por captura    
    NVIC->ISER[0] = (uint32_t)(1 << TIM2_IRQn);     // Hab. IRQ do TIM2 na NVIC

    // Config. o modo one-pulse
    TIM2->CR1 = TIM_CR1_OPM;

    // Configura o SysTick para interrupcao a cada 10ms
    SysTick->LOAD = 80e3;   // Carrega o valor de contagem (10ms)
	SysTick->VAL = 0;       // Limpa o valor da contagem
    SysTick->CTRL = 0b111;  // Clock do processador sem dividir, H? ab. IRQ e SysTick

    createQueue(&queue);

    while (1);
    return 0;
}

// Verifica se o codigo esta vazio.
bool isCodeEmpty(int code[CODE_LENGTH]) {
    for (int i = 0; i < CODE_LENGTH; ++i) {
        if (code[i] != 0)
            return false;
    }
    return true;
}

// Funcao de decadastramento
void unregister() {
    if (team_idx > 0)
        team_idx--;

    for (int i = 0;i < CODE_LENGTH;i++) {
        codes[team_idx][i] = 0;
    }
}

// Compara o codigo do buffer com os coigos registrados.
void compareCodes() {
    if (isCodeEmpty(buffer))  return;
    //EnviaCod_USART(buffer);
    //EnviaStr_USART("\n");
    for (int i = 0; i < team_idx; ++i) {
        bool match = true;
        if (!isCodeEmpty(codes[i])) {
            for (int j = 0; (j < CODE_LENGTH && codes[i][j] != 0); ++j) {
                // Verifica se o valor do buffer esta dentro da tolerancia de 25%
                if ((buffer[j] < codes[i][j] * 0.75 || buffer[j] > codes[i][j] * 1.25) ) {
                    match = false;
                    break;
                }
            }
            if (match) {
                if(i == 0) {                                 // Responsavel por desenfileirar o time que esta sendo atendido
                    deQueue(&queue);
                }
                else if(checkQueue(&queue, i) && !isFull(&queue)) {  
                    enQueue(&queue, i);
                    //EnviaStr_USART("Time ");
                    //EnviaNum_USART(i);
                    //EnviaStr_USART(" chamado\n");
                }
                return;
            }
        }
    }
}

// Processa o sinal recebido.
void process_signal() {
    // MODO DE APRENDIZADO
    if (learning_mode) {    
        if (sgn_duration > 10000) {         // Se o pulso for maior que 10ms, descarta o sinal e finaliza o aprendizado
            EXTI1_IRQHandler();
            return;
        }
        if (sgn_idx < CODE_LENGTH) {    
            codes[team_idx][sgn_idx++] = sgn_duration;
        }
    } 
    // MODO DE OPERACAO
    else  {
        if (sgn_duration > 10000) {
            compareCodes();     
            for (int k = 0; k < CODE_LENGTH; k++) {
                buffer[k] = 0;
            }
            sgn_duration = 0;
            t_ini = 0;  
            sgn_idx = 0;
            return;
        }
    
        if (sgn_idx > CODE_LENGTH) {        // Se o buffer estiver cheio, descarta o sinal 
            sgn_idx = 0;
        }
        buffer[sgn_idx++] = sgn_duration;
    }
}

// Gerencia a interrupcao do TIM2. Chamada nas bordas de descida e subida do sinal.
void TIM2_IRQHandler() {
    if (TIM2->SR & TIM_SR_UIF) {        // Verifica se a IRQ foi disparada pelo overflow
        TIM2->SR &= ~TIM_SR_UIF;        // Apaga flag sinalizadora da IRQ
        sgn_duration = 0;
        t_ini = 0;    
        return;
    }    

    // Se o contador estiver zerado e for borda de descida
    if (TIM2->CCR1 == 0 && (TIM2->CCER & TIM_CCER_CC1P)) { 
        sgn_duration = 0;
        t_ini = 0;
        TIM2->CR1 |= TIM_CR1_CEN;       // Habilita o contador
    } else {
        // Se não, calcula a duracao do pulso
        sgn_duration = TIM2->CCR1 - t_ini;
        t_ini = TIM2->CCR1;
    }

    TIM2->SR &= ~TIM_SR_CC1IF;          // Apaga flag sinalizadora da IRQ
    TIM2->CCER ^= TIM_CCER_CC1P;        // Inverte o sinal de captura Ex.: Se era borda de descida, passa a ser de subida

    if (sgn_duration > 150) {           // Somente considera pulsos com duracao maior que 300us
        process_signal();
    }
}

// Gerencia a interrupcao do EXTI1. Chamada quando o botao de aprendizado e pressionado.
// Habilita/desabilita o modo de aprendizado.
void EXTI1_IRQHandler() {
    EXTI->PR = EXTI_PR_PIF1;        // Apaga flag sinalizadora da IRQ
    EXTI->IMR &= ~EXTI_IMR_IM1;     // Desabilita mascara de interrup. do EXTI1

    antibounce_delay = 25;          // 250ms

    if (learning_mode && !isCodeEmpty(codes[team_idx])) {
        //EnviaStr_USART("Time ");
        //EnviaNum_USART(team_idx);
        //EnviaStr_USART(" cadastrado\n");
        //EnviaCod_USART(codes[team_idx]);
        //EnviaStr_USART("\n");
        team_idx++;
    }
    sgn_idx = 0;
    learning_mode = !learning_mode;
    GPIOA->ODR ^= (1<<3);           // Inverte o estado do LED - Modo aprendizagem: aceso
}

// Gerencia a interrupcao do EXTI2. Chamada quando o botao de remoção de team e pressionado.
void EXTI2_IRQHandler() {
    EXTI->PR = EXTI_PR_PIF2;        // Apaga flag sinalizadora da IRQ
    EXTI->IMR &= ~EXTI_IMR_IM2;     // Desabilita mascara de interrup. do EXTI2

    antibounce_delay = 25;          // 250ms
    unregister();
}

// Gerencia a interrupcao do SysTick. Chamada a cada 10ms.
void SysTick_Handler() {
    if (antibounce_delay > 0) {
        antibounce_delay--;
    } else {
        EXTI->IMR |= EXTI_IMR_IM1 | EXTI_IMR_IM2; // Hab. mascara de interrup. do EXTI1 e EXTI2        
    }
}

// Gerencia a interrupcao da USART1. Chamada quando um dado e recebido.
void USART1_IRQHandler() {
    char tx_data[50] = "";
    char aux[50] = "";
    if (USART1->SR & USART_SR_RXNE) {
        char rx_data = USART1->DR;        
        if (rx_data == 'd') {
            get_queue_data(&queue, tx_data);
            sprintf(aux, "%d;", team_idx);
            strcat(tx_data, aux);               // Concatena o numero de times cadastrados
            EnviaStr_USART(tx_data);            // Envia os dados da fila
        }
    }
}