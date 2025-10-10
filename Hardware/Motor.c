#include "stm32f10x.h"                  // Device header
#include "PWM.h"

void Motor_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);    
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);                        
    
    // 初始化时先关闭电机
    GPIO_ResetBits(GPIOA, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);
    
    PWM_Init();
}

/* 左电机 (PB11 → PWM_CH4, PA6=BIN1, PA7=BIN2) */
void Motor_SetLeftSpeed(int8_t Speed)
{
    if (Speed > 0)     // 正转
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_6);    // BIN1=1
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);  // BIN2=0
        PWM_SetCompare4(Speed);
    }
    else if (Speed < 0) // 反转
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_6);  // BIN1=0
        GPIO_SetBits(GPIOA, GPIO_Pin_7);    // BIN2=1
        PWM_SetCompare4(-Speed);
    }
    else                // 刹车
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_6 | GPIO_Pin_7);
        PWM_SetCompare4(0);
    }
}

/* 右电机 (PB10 → PWM_CH3, PA4=AIN1, PA5=AIN2) */
void Motor_SetRightSpeed(int8_t Speed)
{
    if (Speed > 0)     // 正转
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_4);    // AIN1=1
        GPIO_ResetBits(GPIOA, GPIO_Pin_5);  // AIN2=0
        PWM_SetCompare3(Speed);
    }
    else if (Speed < 0) // 反转
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_4);  // AIN1=0
        GPIO_SetBits(GPIOA, GPIO_Pin_5);    // AIN2=1
        PWM_SetCompare3(-Speed);
    }
    else                // 刹车
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_4 | GPIO_Pin_5);
        PWM_SetCompare3(0);
    }
}
