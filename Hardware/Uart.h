#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"

// 初始化 USART2
void USART2_Init(uint32_t baudrate);

// 串口发送
void USART2_SendChar(uint8_t c);
void USART2_SendString(char *str);

// 获取一帧完整数据，返回1表示成功，0表示没有新帧
uint8_t USART2_GetFrame(char *out);

#endif
