#include "stm32f10x.h"
#include "Delay.h"

// 红外读取宏定义（所有引脚都在GPIOA）
#define Infrared(Pin) GPIO_ReadInputDataBit(GPIOA, Pin)

void Infrared_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 开启GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 2. 配置所有引脚为上拉输入模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3; // PA0-PA3
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 3. 启用内部上拉电阻
    GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);
    
    // 4. 初始化后短暂延时确保稳定
    Delay_ms(10); // 需要Delay.h支持
}
