#include "stm32f10x.h"
#include "UART.h"
#include <string.h>

#define RX_BUFFER_SIZE 128
static char RxBuffer[RX_BUFFER_SIZE];
static uint8_t RxIndex = 0;
volatile uint8_t FrameReady = 0;

void USART2_Init(uint32_t baudrate) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // PA2 = TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA3 = RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 USART2
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART2, &USART_InitStructure);

    // 使能接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    // NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);

    RxIndex = 0;
    FrameReady = 0;
}

// 串口发送
void USART2_SendChar(uint8_t c) {
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, c);
}
void USART2_SendString(char *str) {
    while (*str) {
        USART2_SendChar(*str++);
    }
}

// 获取完整帧
uint8_t USART2_GetFrame(char *out) {
    if (!FrameReady) return 0;
    strcpy(out, RxBuffer);   // 拷贝一份出来
    FrameReady = 0;
    return 1;
}

// USART2 中断服务函数
void USART2_IRQHandler(void) {
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        char data = USART_ReceiveData(USART2) & 0xFF;

        if (RxIndex == 0 && data != '$') {
            // 必须以 $ 开始
            return;
        }

        if (RxIndex < RX_BUFFER_SIZE - 1) {
            RxBuffer[RxIndex++] = data;
            if (data == '#') {   // 收到一帧结束
                RxBuffer[RxIndex] = '\0';
                FrameReady = 1;
                RxIndex = 0;
            }
        } else {
            RxIndex = 0; // 缓冲区溢出重置
        }
    }
}
