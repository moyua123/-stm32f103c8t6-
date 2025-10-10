#include "stm32f10x.h"                  // Device header
#include "Motor.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>

#define MOTOR_INVERT 1

// 8 路传感器转成二进制
uint8_t sensor_bits = 0;

// 基础速度 & PID 系数
#define BASE_SPEED 70
#define KP 20
#define KI 0
#define KD 8

int last_error = 0;


static int lastError = 0;
static int integral  = 0;

static inline int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
void TraceLine_PID(uint8_t sensorBits)
{
    // ===== 终点判定 =====
    if (sensorBits == 0x00) {               // 全黑 -> 终点停车
        Motor_SetLeftSpeed(0);
        Motor_SetRightSpeed(0);
        OLED_ShowString(3, 1, "End     ");
        lastError = 0;
        integral  = 0;
        return;
    }

    // ===== 计算误差 error =====
    // L1..L4 = bit7..bit4(黑=1), R1..R4 = bit3..bit0(黑=1)
    // 左为负，右为正
    int error = 0;
    if (sensorBits & 0x80) error -= 4;  // L1
    if (sensorBits & 0x40) error -= 3;  // L2
    if (sensorBits & 0x20) error -= 2;  // L3
    if (sensorBits & 0x10) error -= 1;  // L4
    if (sensorBits & 0x08) error += 1;  // R1
    if (sensorBits & 0x04) error += 2;  // R2
    if (sensorBits & 0x02) error += 3;  // R3
    if (sensorBits & 0x01) error += 4;  // R4

    // 如果全白 error==0，可选择保持 lastError 的方向（这里允许 PID 自然纠偏，不停车）
    if (sensorBits == 0xFF) {
        error = lastError;  // 保持上一次方向
    }

    // ===== PID =====
    integral += error;
    integral = clamp(integral, -200, 200);

    int derivative = error - lastError;
    lastError = error;

    int correction = KP * error + KI * integral + KD * derivative;

    int leftSpeed  = BASE_SPEED - correction;
    int rightSpeed = BASE_SPEED + correction;

    // 限幅
    leftSpeed  = clamp(leftSpeed,  -100, 100);
    rightSpeed = clamp(rightSpeed, -100, 100);

    // ===== 电机输出（考虑反接） =====
#if MOTOR_INVERT
    Motor_SetLeftSpeed(-leftSpeed);
    Motor_SetRightSpeed(-rightSpeed);
#else
    Motor_SetLeftSpeed(leftSpeed);
    Motor_SetRightSpeed(rightSpeed);
#endif

    // OLED 调试显示
    OLED_ShowString(4, 1, "E:");
    OLED_ShowNum(4, 3, error, 3);
    OLED_ShowString(4, 7, "L:");
    OLED_ShowNum(4, 9, leftSpeed, 3);
    OLED_ShowString(4, 13, "R:");
    OLED_ShowNum(4, 15, rightSpeed, 3);
}

// === 串口数据解析 ===
void ParseFrame(char *frame) {
    if (frame[0] == '$' && frame[1] == 'D') {
        int x[8];
        if (sscanf(frame, "$D,x1:%d,x2:%d,x3:%d,x4:%d,x5:%d,x6:%d,x7:%d,x8:%d#",
                   &x[0],&x[1],&x[2],&x[3],&x[4],&x[5],&x[6],&x[7]) == 8) {

            // OLED 显示原始值
            OLED_ShowString(2,1,"S:");
            for (int i=0;i<8;i++) {
                OLED_ShowNum(2,3+i,x[i],1);
            }

            // 转成二进制
            sensor_bits = 0;
            for (int i = 0; i < 8; i++) {
                sensor_bits <<= 1;
                if (x[i]) sensor_bits |= 1;
            }

            // 调用 PID
            TraceLine_PID(sensor_bits);
        }
    }
}
