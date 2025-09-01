#include "stm32f10x.h"                  // Device header
#include "Delay.h"
/******************* TB6612 引脚定义 *******************/
// 右电机 A 通道
#define AIN1_Pin    GPIO_Pin_4
#define AIN2_Pin    GPIO_Pin_5
#define AIN_GPIO    GPIOA

// 左电机 B 通道
#define BIN1_Pin    GPIO_Pin_6
#define BIN2_Pin    GPIO_Pin_7
#define BIN_GPIO    GPIOA

// STBY 引脚（使用 PA8）
#define STBY_Pin    GPIO_Pin_8
#define STBY_GPIO   GPIOA

/******************* 初始化函数 *******************/
void PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 打开时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* 配置方向引脚 AIN1/AIN2, BIN1/BIN2 + STBY 为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = AIN1_Pin | AIN2_Pin | BIN1_Pin | BIN2_Pin | STBY_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 上电安全：默认全刹车 (0,0)，STBY 先拉低，电机不使能 */
    GPIO_ResetBits(AIN_GPIO, AIN1_Pin | AIN2_Pin);
    GPIO_ResetBits(BIN_GPIO, BIN1_Pin | BIN2_Pin);
    GPIO_ResetBits(STBY_GPIO, STBY_Pin);   // 待机状态
	Delay_ms(10);
    /* PWM 引脚 PB10 (TIM2_CH3), PB11 (TIM2_CH4) 先设为 GPIO 输出，拉低 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11);

    /* 配置 TIM2 */
    TIM_InternalClockConfig(TIM2);
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;     // ARR = 99
    TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;   // PSC = 35
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    /* 配置 PWM 输出模式 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

    TIM_OCInitStructure.TIM_Pulse = 0;   // 默认占空比 0
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* 部分重映射：TIM2_CH3->PB10, TIM2_CH4->PB11 */
    GPIO_PinRemapConfig(GPIO_PartialRemap2_TIM2, ENABLE);

    /* 现在再把 PB10/PB11 配置成复用推挽输出 (AF_PP) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 启动 TIM2 */
    TIM_Cmd(TIM2, ENABLE);

    /* 最后再拉高 STBY，允许电机工作 */
    GPIO_SetBits(STBY_GPIO, STBY_Pin);
}

/******************* PWM 设置 *******************/
void PWM_SetCompare3(uint16_t Compare) { TIM_SetCompare3(TIM2, Compare); }  // 右电机速度
void PWM_SetCompare4(uint16_t Compare) { TIM_SetCompare4(TIM2, Compare); }  // 左电机速度

