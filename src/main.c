#include "stm32f1xx.h" // Biblioteca STM

unsigned int periodo_d, tini_d;
unsigned int periodo_s, tini_s;

unsigned char tx_str[10]; // String para transmissao na usart 

void EnviaDado_USART(char tx_dado)  {
    while (!(USART1->SR & USART_SR_TXE)); // Aguarda reg. de dado Tx estar vazio
    USART1->DR = tx_dado;
}

void EnviaStr_USART(char *string) {
    while(*string){
        EnviaDado_USART(*string);
        string++;
    }
}

void int2str(unsigned int valor) { /* Converte um valor inteiro em string */
    tx_str[3] = 0;                  // fim do vetor
    tx_str[2] = 48 + valor % 10;    // 48 é o valor do caracter 0 em ASCII
    valor /= 10;                    // unidades
    tx_str[1] = 48 + valor % 10;
    valor = valor / 10;             // dezenas
    tx_str[0] = 48 + valor % 10;    // centenas
}

void TIM2_IRQHandler(void) { /* Trata da IRQ do TIM2 */
    TIM2->SR &= ~TIM_SR_CC1IF;              // Apaga flag sinalizadora da IRQ
    if((TIM2->CCER & TIM_CCER_CC1P)) {      // Estava sensiível na borda de descida?
        GPIOC->ODR &= ~(1<<13);             // LED onboard acende

        tini_d = TIM2->CCR1;
        periodo_s = TIM2->CCR1 - tini_s;    // Determina o período do sinal alto
        TIM2->CCER &= ~TIM_CCER_CC1P;       // Sensí­vel na borda de subida da entrada
    } else {
        GPIOC->ODR |= (1<<13);              // LED onboard apaga

        tini_s = TIM2->CCR1;
        periodo_d = TIM2->CCR1 - tini_d;    // Determina o período do sinal baixo
        TIM2->CCER |= TIM_CCER_CC1P;        // Sensível na borda de descida da entrada
    }
}

void IniciaCapturaTIM2(void) {
    /* Config. TIM2 com entrada de captura no canal 1 (PA0) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;         // Habilita clock do TIM2 do bus APB1
    TIM2->ARR = 0xFFFF;                         // Registrador de auto-carregamento (1kHz -> perÃ­odo max. 1ms)
    TIM2->PSC = 79;                             // Prescaler divide por 80 (100 kHz)
    TIM2->CNT = 0;                              // Valor inicial da contagem
    
    // Ativa a entrada
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;            // Sem filtro na entrada, sem prescaler na entrada, TI1 como entrada
    TIM2->CCER |= TIM_CCER_CC1P;                // SensÃ­vel na borda de descida da entrada
    TIM2->CCER |= TIM_CCER_CC1E;                // Habilita a captura no CH1
    TIM2->SR &= ~TIM_SR_CC1IF;                  // Apaga flag sinalizadora da IRQ
    TIM2->DIER |= TIM_DIER_CC1IE;               // Habilita IRQ por captura
    TIM2->CR1 |= TIM_CR1_CEN;                   // Hab. contagem
    //NVIC->IP[TIM2_IRQn] = 35;                 // Config. prioridade do TIM2
    NVIC->ISER[0] = (uint32_t)(1 << TIM2_IRQn); // Hab. IRQ do TIM2 na NVIC
}

void SysTick_Handler (void) {  // A cada 1 s faz uma nova captura
    unsigned int a;
    IniciaCapturaTIM2();    // Inicia a captura do perÃiodo
    GPIOA->ODR |= (1<<1);   // Ativa o trigger do sonar (PA1)... 
    for(a=0; a<19; a++)
        __asm("nop"); 
    GPIOA->ODR &= ~(1<<1);  // ... por cerca de 10us
}

void ConfigSystick(void) {
    SysTick->LOAD = 8e6;        // Velocidade do clock (default BluePill: HSI com 8MHz)
	SysTick->VAL = 0;		    // Limpa o valor da contagem
    SysTick->CTRL = 0b111; 	    // Clock do processador sem dividir, Hab. IRQ e SysTick
}

int main(void) { 
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN  | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN; // Habilita clock do barramento APB2

    GPIOC->CRH |= GPIO_CRH_MODE13_1;                        // Configura pino PC13 como saida open-drain 2 MHz
    GPIOA->CRL = (GPIOA->CRL & 0xFFFFFF0F) | 0x00000030;    // 0b0111 PA1 como sai­da push-pull e alta velocidade
    
    // PA9 como saida push-pull em funcao alt. (Tx da USART1)
    GPIOA->CRH = (GPIOA->CRH & 0xFFFFFF0F) | 0x000000B0; // 0b1011=0xB

    /* Config USART1 para Tx e Rx sem IRQ */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;           // Hab. clock para USART1
    USART1->BRR  = 8000000/9600;                    // Define baudrate = 9600 baud/s (APB2_clock = 8 MHz)     
    USART1->CR1 |= (USART_CR1_RE | USART_CR1_TE);   // Hab. RX e TX
    USART1->CR1 |= USART_CR1_UE;                    // Hab USART1

    IniciaCapturaTIM2();  // Faz uma primeira captura   
    ConfigSystick();

    for(;;); /* Loop infinito */
}
