#include <stdbool.h>
#include <stdio.h>

#include "stm32f1xx.h"

#define STR_SIZE 5

bool learning_mode = false;
int sgn_duration, t_ini;
int antibounce_delay;
int sgn_idx, team_idx, codes[10][200], buffer[200];

char tx_str[10];

// Prototipos das funcoes
void ConfigTIM2();
void ConfigSystick();

// Funcoes de debug
void EnviaStr_USART(char *string);
void EnviaNum_USART(int valor);
void int2str(int valor);
void EXTI1_IRQHandler (void);

int main() {
    // Habilita clock do barramento APB2
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN  | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN; 

    
    GPIOC->CRH |= GPIO_CRH_MODE13_1;                        // Configura pino PC13 como saida open-drain 2 MHz

    // Configura pino PA1 como entrada e PA2 como saida                     
    GPIOA->CRL = (GPIOA->CRL & 0xFFFFF00F) | (0x8U << (4 * 1)) | (0x3U << (4 * 2));
    GPIOA->ODR |=  GPIO_ODR_ODR1;                           // Habilitar pull-up no pino PA1
    

    // Configura o USART1 para Tx e Rx sem IRQ
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFFF0F) | 0x000000B0;    // PA9 como saida push-pull em funcao alt. (Tx da USART1) 0b1011=0xB
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;                   // Hab. clock para USART1
    USART1->BRR  = 8000000/9600;                            // Define baudrate = 9600 baud/s (APB2_clock = 8 MHz)     
    USART1->CR1 |= (USART_CR1_RE | USART_CR1_TE);           // Hab. RX e TX
    USART1->CR1 |= USART_CR1_UE;                            // Hab USART1

    // Configura o PA1 com interrupcao no EXTI1
    AFIO->EXTICR[0] = AFIO_EXTICR1_EXTI1_PA;                // Seleciona PA1 para EXTI1
    EXTI->FTSR = EXTI_FTSR_FT1;                             // Sensivel na rampa de descida
    EXTI->IMR = EXTI_IMR_IM1;                               // Hab. mascara de interrup. do EXTI1
    NVIC->ISER[0] = (uint32_t)(1 << EXTI1_IRQn);            // Hab. IRQ do EXTI1 na NVIC

    ConfigSystick();
    ConfigTIM2(); 

    while (1);
 
    return 0;
}

void EnviaStr_USART(char *string) {
    while(*string){
        while (!(USART1->SR & USART_SR_TXE));       // Aguarda reg. de dado Tx estar vazio
        USART1->DR = *string;
        string++; 
    }
}

void int2str(int valor) {
    int j = 0;
    if (valor == 0) {
        tx_str[0] = '0';
        tx_str[1] = '\0';
    } else {
        if (valor < 0) {
            tx_str[0] = '-';
            valor *= -1;
            j++;
        }
        tx_str[STR_SIZE] = '\0'; // Marcar o fim da string
        for (int i = STR_SIZE - 1; i >= j; --i) {
            tx_str[i] = (valor % 10) + '0'; // Converte o digito para char
            valor /= 10;
        }
    }
}

void EnviaNum_USART(int valor) { 
    int2str(valor);
    EnviaStr_USART(tx_str);
}

void ConfigTIM2() {
    /* Config. TIM2 com entrada de captura no canal 1 (PA0) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;             // Habilita clock do T? IM2 do bus APB1
    TIM2->ARR = 0xFFFF;                             // Registrador de auto-carregamento (1kHz -> periodo max. 1ms)
    TIM2->PSC = 15;                                 // Prescaler
    TIM2->CNT = 0;                                  // Zera o contador

    // Config. a entrada
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;                // Sem filtro na entrada, sem prescaler na entrada, TI1 como entrada
    TIM2->CCER |= TIM_CCER_CC1P;                    // Sensivel na borda de descida da entrada
    TIM2->CCER |= TIM_CCER_CC1E;                    // Habilita a captura no CH1
    TIM2->SR &= ~TIM_SR_CC1IF;                      // Apaga flag sinalizadora da IRQ
    TIM2->DIER |= TIM_DIER_CC1IE;                   // Habilita IRQ por captura    
    NVIC->ISER[0] = (uint32_t)(1 << TIM2_IRQn);     // Hab. IRQ do TIM2 na NVIC

    // Config. o modo one-pulse
    TIM2->CR1 = TIM_CR1_OPM;
}

void ConfigSystick() {
    SysTick->LOAD = 80e3;   // Carrega o valor de contagem (10ms)
	SysTick->VAL = 0;       // Limpa o valor da contagem
    SysTick->CTRL = 0b111;  // Clock do processador sem dividir, H? ab. IRQ e SysTick
}

// Gerencia a interrupcao do TIM2. Chamada nas bordas de descida e subida do sinal.
void TIM2_IRQHandler() {
    TIM2->SR &= ~TIM_SR_CC1IF;          // Apaga flag sinalizadora da IRQ
    TIM2->CCER ^= TIM_CCER_CC1P;        // Inverte o sinal de captura Ex.: Se era borda de descida, passa a ser de subida
   
    sgn_duration = TIM2->CCR1 - t_ini;
    t_ini = TIM2->CCR1;

    if (TIM2->CCR1 == 0) {              // Hab. contagem se o contador estiver zerado
        TIM2->CR1 |= TIM_CR1_CEN;
    }

    if (sgn_duration > 150) {           // Somente considera pulsos com duracao maior que 300us
        // Modo de aprendizado
        if (learning_mode) {    
            if (sgn_duration > 10000) { // Se o pulso for maior que 10ms, descarta o sinal e finaliza o aprendizado
                EXTI1_IRQHandler(); 
                return;
            }
            if (sgn_idx < 200) {    
                codes[team_idx][sgn_idx++] = sgn_duration;
            }
        } 
        // Modo de operacao
        else if (sgn_duration < 10000) {            
            if (sgn_idx < 200) {
                buffer[sgn_idx++] = sgn_duration;
            }
        }
    }
}

// Gerencia a interrupcao do EXTI1. Chamada quando o botao de aprendizado e pressionado.
// Habilita/desabilita o modo de aprendizado.
void EXTI1_IRQHandler (void) {
    EXTI->PR = EXTI_PR_PIF1;        // Apaga flag sinalizadora da IRQ
    EXTI->IMR &= ~EXTI_IMR_IM1;     // Desabilita mascara de interrup. do EXTI1

    antibounce_delay = 25; 

    learning_mode = !learning_mode;
    GPIOA->ODR ^= (1<<2);           // Inverte o estado do LED - Modo aprendizagem: aceso

    if (sgn_idx > 0) {        
        sgn_idx = 0;
    }

    if (!learning_mode) {        
        EnviaStr_USART("Time cadastrado:");
        EnviaNum_USART(team_idx);
        EnviaStr_USART("\n");
        EnviaStr_USART("Codigo: ");
        for (int i = 0; codes[team_idx][i] != 0; i++) {
            EnviaNum_USART(codes[team_idx][i]);
            EnviaStr_USART(" ");
        }
        EnviaStr_USART("\n\n");
        team_idx++;
    }
}

// Gerencia a interrupcao do SysTick. Chamada a cada 10ms.
void SysTick_Handler() {
    if (antibounce_delay>0) {
        antibounce_delay--;
    } else {
        EXTI->IMR |= EXTI_IMR_IM1; // Hab. mascara de interrup. do EXTI1
    }
}