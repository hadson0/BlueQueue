#include <stdbool.h>
#include <stdio.h>

#include "stm32f1xx.h"

#define STR_LENGTH 5
#define CODE_LENGTH 200
#define SIZE_QUEUE 10

bool learning_mode = false;
int sgn_duration, t_ini;
int antibounce_delay;
int sgn_idx, team_idx, codes[10][CODE_LENGTH], buffer[CODE_LENGTH], fila[10] = {20};
int topo = -1, fundo = -1;

char tx_str[10];

// Prototipos das funcoes
void ConfigTIM2();
void ConfigSystick();
void TIM2_IRQHandler();
void EXTI1_IRQHandler();
void SysTick_Handler();
void compareCodes();

// Funcoes de debug
void EnviaStr_USART(char *string);
void EnviaNum_USART(int valor);
void int2str(int valor);
void EXTI1_IRQHandler (void);
void EXTI2_IRQHandler(void);

// Funcoes de fila
int checkQueue(int team);
int deQueue();
void enQueue(int team);
int isFull();
int isEmpty();


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
    USART1->CR1 |= USART_CR1_UE;                            // Hab USART1

    // Configura o PA1|PA2 com interrupcao no EXTI1|EXTI2
    AFIO->EXTICR[0] = AFIO_EXTICR1_EXTI1_PA | AFIO_EXTICR1_EXTI2_PA;                 // Seleciona PA1 para EXTI1
    EXTI->FTSR = EXTI_FTSR_FT1 | EXTI_FTSR_FT2;                                     // Sensivel na rampa de descida
    EXTI->IMR = EXTI_IMR_IM1 | EXTI_IMR_IM2;                                        // Hab. mascara de interrup. do EXTI1
    NVIC->ISER[0] = (uint32_t)(1 << EXTI1_IRQn) | (uint32_t)(1 << EXTI2_IRQn);      // Hab. IRQ do EXTI1 na NVIC

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
        tx_str[STR_LENGTH] = '\0'; // Marca o fim da string
        for (int i = STR_LENGTH - 1; i >= j; --i) {
            tx_str[i] = (valor % 10) + '0'; // Converte o digito para char
            valor /= 10;
        }
    }
}

void EnviaNum_USART(int valor) { 
    int2str(valor);
    EnviaStr_USART(tx_str);
}

void EnviaCod_USART(int code[CODE_LENGTH]) {
    EnviaStr_USART("Codigo: ");
    for (int i = 0; code[i] != 0; i++) {
        EnviaNum_USART(code[i]);
        EnviaStr_USART(" ");
    }
    EnviaStr_USART("\n");
}

void ConfigTIM2() {
    /* Config. TIM2 com entrada de captura no canal 1 (PA0) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;             // Habilita clock do TIM2 do bus APB1
    TIM2->ARR = 0xFFFF;                             // Registrador de auto-carregamento (1kHz -> periodo max. 1ms)
    TIM2->PSC = 15;                                 // Prescaler
    TIM2->CNT = 0;                                  // Zera o contador

    // Config. a entrada
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;                // Sem filtro na entrada, sem prescaler na entrada, TI1 como entrada
    TIM2->CCER |= TIM_CCER_CC1P;                    // Sensivel na borda de descida da entrada
    TIM2->CCER |= TIM_CCER_CC1E;                    // Habilita a captura no CH1
    TIM2->SR &= ~TIM_SR_CC1IF;                      // Apaga flag sinalizadora da IRQ
    TIM2->DIER |= TIM_DIER_UIE | TIM_DIER_CC1IE;                   // Habilita IRQ por captura    
    NVIC->ISER[0] = (uint32_t)(1 << TIM2_IRQn);     // Hab. IRQ do TIM2 na NVIC

    // Config. o modo one-pulse
    TIM2->CR1 = TIM_CR1_OPM;
}

void ConfigSystick() {
    SysTick->LOAD = 80e3;   // Carrega o valor de contagem (10ms)
	SysTick->VAL = 0;       // Limpa o valor da contagem
    SysTick->CTRL = 0b111;  // Clock do processador sem dividir, H? ab. IRQ e SysTick
}

// Verifica se o cÃ³digo estÃ¡ vazio.
bool isCodeEmpty(int code[CODE_LENGTH]) {
    for (int i = 0; i < CODE_LENGTH; ++i) {
        if (code[i] != 0) {
            return false;
        }
    }
    return true;
}

// Compara o codigo do buffer com os coigos registrados.
void compareCodes() {
    if (isCodeEmpty(buffer))  return;

    EnviaCod_USART(buffer);
    for (int i = 0; i < team_idx; ++i) {
        bool match = true;

        if (!isCodeEmpty(codes[i])) {
            for (int j = 0; (j < CODE_LENGTH && codes[i][j] != 0); ++j) {
                // Verifica se o valor do buffer esta dentro da tolerancia de 25%
                if ((buffer[j] < codes[i][j] * 0.75 || buffer[j] > codes[i][j] * 1.25) ) {
                    match = false;
                    EnviaStr_USART("Diferenca de ");
                    EnviaNum_USART((codes[i][j] - buffer[j]) * 100 / codes[i][j]);
                    EnviaStr_USART("% no codigo, esperado ");
                    EnviaNum_USART(codes[i][j]);                 
                    EnviaStr_USART(" e detectado ");
                    EnviaNum_USART(buffer[j]);
                    EnviaStr_USART("\n\n");
                    break;
                }
            }
            if (match) {
                EnviaStr_USART("team reconhecido:");
                EnviaNum_USART(i);
                EnviaStr_USART("\n\n");

                if(checkQueue(i)){                      //Checa se ja ta na fila
                    enQueue(i);
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
        EnviaStr_USART("team cadastrado:");
        EnviaNum_USART(team_idx);
        EnviaStr_USART("\n");
        EnviaCod_USART(codes[team_idx]);
        EnviaStr_USART("\n");
        team_idx++;
    }
    sgn_idx = 0;
    learning_mode = !learning_mode;
    GPIOA->ODR ^= (1<<3);           // Inverte o estado do LED - Modo aprendizagem: aceso
}

// Gerencia a interrupcao do EXTI2. Chamada quando o botao de remoção de team e pressionado
void EXTI2_IRQHandler() {
    EXTI->PR = EXTI_PR_PIF2;        // Apaga flag sinalizadora da IRQ
    EXTI->IMR &= ~EXTI_IMR_IM2;     // Desabilita mascara de interrup. do EXTI2

    antibounce_delay = 25;          // 250ms
    
    if(team_idx > 0)
        team_idx--;

    for(int i = 0;i < CODE_LENGTH;i++) {
        codes[team_idx][i] = 0;
    }
}


// Gerencia a interrupcao do SysTick. Chamada a cada 10ms.
void SysTick_Handler() {
    if (antibounce_delay > 0) {
        antibounce_delay--;
    } else {
        EXTI->IMR |= EXTI_IMR_IM1 | EXTI_IMR_IM2; // Hab. mascara de interrup. do EXTI1 e EXTI2        
    }
}

//Funcoes da fila

int isFull() {
  if ((topo == fundo + 1) || (topo == 0 && fundo == SIZE_QUEUE - 1)) return 1;
  return 0;
}

int isEmpty() {
  if (topo == -1) return 1;
  return 0;
}

void enQueue(int team) {
  if (isFull())
    EnviaStr_USART("Fila cheia \n");
  else {
    if (topo == -1) topo = 0;
    fundo = (fundo + 1) % SIZE_QUEUE;
    fila[fundo] = team;
    EnviaStr_USART("Inserido ");
  }
}

int deQueue() {
  int team;
  if (isEmpty()) {
    EnviaStr_USART("Fila vazia \n");
    return (-1);
  } else {
    team = fila[topo];
    if (topo == fundo) {
      topo = -1;
      fundo = -1;
    } 
    else {
      topo = (topo + 1) % SIZE_QUEUE;
    }
    EnviaStr_USART("team deletado ");
    return (team);
  }
}

int checkQueue(int team){
    if(isEmpty()){
        return 1;
    }
    for (int i = topo; i != fundo; i = (i + 1) % SIZE_QUEUE) {
        if(fila[i] == i){
            return 1;
        }
    }
    return 0;
}


