// TraceLine_PID.c
#include "stm32f10x.h"                  // Device header
#include "Motor.h"
#include "Infrared.h"
#include "Delay.h"
#include "OLED.h"

// 红外传感器引脚定义
#define ll GPIO_Pin_0
#define l  GPIO_Pin_1
#define r  GPIO_Pin_2
#define rr GPIO_Pin_3
#define Infrared(GPIOX,Pin) GPIO_ReadInputDataBit(GPIOX, Pin)

// 读取红外传感器状态（黑=1，白=0）
static uint8_t ReadLineSensors(void) {
    uint8_t ll_val = !Infrared(GPIOA, ll);
    uint8_t l_val  = !Infrared(GPIOA, l);
    uint8_t r_val  = !Infrared(GPIOA, r);
    uint8_t rr_val = !Infrared(GPIOA, rr);
    return (ll_val << 3) | (l_val << 2) | (r_val << 1) | rr_val;
}

// 差速控制参数
#define BASE_SPEED 80
#define KP 25   // 比例系数，可调节

void TraceLine_PID(void) {
    uint8_t sensorState = ReadLineSensors();

    // 如果传感器全黑，停车
    if(sensorState == 0x00) {
        Motor_SetLeftSpeed(0);
        Motor_SetRightSpeed(0);
        OLED_ShowString(3, 9, "Stop");
        return;
    }
    
    // 计算偏差 error
    // 左右传感器组合形成 [-3,3] 偏差
    int error = 0;
    if(sensorState & 0x8) error -= 3; // LL
    if(sensorState & 0x4) error -= 2; // L
    if(sensorState & 0x2) error += 2; // R
    if(sensorState & 0x1) error += 3; // RR

    int leftSpeed = BASE_SPEED - KP * error;
    int rightSpeed = BASE_SPEED + KP * error;

    // 限制速度在 -100~100
    if(leftSpeed > 100) leftSpeed = 100;
    if(leftSpeed < -100) leftSpeed = -100;
    if(rightSpeed > 100) rightSpeed = 100;
    if(rightSpeed < -100) rightSpeed = -100;

    // **左右电机反接处理**
    Motor_SetLeftSpeed(-leftSpeed);
    Motor_SetRightSpeed(-rightSpeed);

    // OLED 显示动作
    if(error == 0) OLED_ShowString(3, 9, "Forward");
    else if(error < 0) OLED_ShowString(3, 9, "Left");
    else OLED_ShowString(3, 9, "Right");
}
