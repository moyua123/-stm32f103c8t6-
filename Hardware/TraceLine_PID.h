#ifndef __TraceLine_PID_H
#define __TraceLine_PID_H


// ====================== 参数配置 ======================

// 电机是否整体反接（左右都反接）
// 1 = 取反输出（你的硬件情况）; 0 = 不取反
#define MOTOR_INVERT 1

// 基础速度 & PID 系数
#define BASE_SPEED 70
#define KP 20
#define KI 0
#define KD 8

void TraceLine_PID(uint8_t sensors);
void ParseFrame(char *frame);

#endif
