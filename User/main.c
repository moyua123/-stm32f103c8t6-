#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Key.h"
#include "CarControl.h"
#include "Serial.h"
#include "Servo.h"
#include "Ultrasound.h"
#include "I2C.h"
#include "VoiceControl.h"
#include "TraceLine_PID.h"
#include "UART.h"   // USART2 的初始化和中断
#include <string.h>
#include <stdio.h>

// ================== 工作模式定义 ==================
typedef enum {
    MODE_REMOTE,     // 遥控模式
    MODE_AVOID,      // 避障模式
    MODE_TRACE,      // 循迹模式
    MODE_COUNT
} WorkMode;

// ================== 全局变量 ==================
uint16_t KeyNum;
WorkMode currentMode = MODE_REMOTE; // 默认遥控模式

uint16_t dis = 0;
uint16_t disr = 0;
uint16_t disl = 0;

char FrameBuf[128]; // 循迹模块接收缓冲区

// ================== 函数声明 ==================
void DisplaySystemStatus(void);
void DisplayCurrentAction(char* action);
void ObAv(void);
void SwitchMode(WorkMode newMode, char* source);

// ================== 显示系统状态 ==================
void DisplaySystemStatus(void) {
    OLED_Clear();
    OLED_ShowString(1, 1, "SmartCar System");
    
    switch(currentMode) {
        case MODE_REMOTE: OLED_ShowString(2, 1, "Mode: Remote"); break;
        case MODE_AVOID:  OLED_ShowString(2, 1, "Mode: Avoid");  break;
        case MODE_TRACE:  OLED_ShowString(2, 1, "Mode: Trace");  break;
        default:          OLED_ShowString(2, 1, "Mode: Unknown"); break;
    }
    
    OLED_ShowString(3, 1, "Action: ");
}

// ================== 显示当前动作 ==================
void DisplayCurrentAction(char* action) {
    OLED_ShowString(3, 9, action);
}

// ================== 避障逻辑 ==================
void ObAv(void) 
{
    int front, left, right;

    Servo_SetAngle(90);
    Delay_ms(150);
    front = Test_Distance();

    if(front > 45) {
        Go_Ahead();
        return;
    }

    Car_Stop();
    Go_Back();
    Delay_ms(300);
    Car_Stop();

    Servo_SetAngle(45);
    Delay_ms(200);
    right = Test_Distance();

    Servo_SetAngle(135);
    Delay_ms(200);
    left = Test_Distance();

    Servo_SetAngle(90);
    Delay_ms(100);

    if(right > left && right > 45) {
        Turn_Right();
        while(Test_Distance() < 45) Delay_ms(50);
        Go_Ahead();
    }
    else if(left > 45) {
        Turn_Left();
        while(Test_Distance() < 45) Delay_ms(50);
        Go_Ahead();
    }
    else {
        Go_Back();
        Delay_ms(600);
        Car_Stop();
    }
}

// ================== 模式切换统一函数 ==================
void SwitchMode(WorkMode newMode, char* source)
{
    currentMode = newMode;
    Car_Stop();
    Servo_SetAngle(90);
    DisplaySystemStatus();

    switch(currentMode) {
        case MODE_REMOTE:
            DisplayCurrentAction("Stop");
            Serial_SendString("Mode: Remote (");
            break;
        case MODE_AVOID:
            DisplayCurrentAction("Avoiding");
            Serial_SendString("Mode: Avoid (");
            break;
        case MODE_TRACE:
            DisplayCurrentAction("Tracing");
            Serial_SendString("Mode: Trace (");
            break;
        default:
            DisplayCurrentAction("Unknown");
            Serial_SendString("Mode: Unknown (");
            break;
    }

    Serial_SendString(source);
    Serial_SendString(")\r\n");
}

// ================== 主函数 ==================
int main(void) {
    Car_Init();
    OLED_Init();
    Key_Init();
    Serial_Init();       // 蓝牙串口1
    Servo_Init();
    Ultrasound_Init();
    USART2_Init(115200); // 循迹模块串口2
	VoiceSwitch_Init();
    DisplaySystemStatus();
    DisplayCurrentAction("Stop");
    Car_Stop();
    
    Serial_SendString("SmartCar Ready!\r\n");
    USART2_SendString("$0,0,1#\r\n");

    while (1) {
        KeyNum = Key_GetNum();
        if(KeyNum == 1) {
            WorkMode nextMode = (WorkMode)((currentMode + 1) % MODE_COUNT);
            SwitchMode(nextMode, "Key");
        }

        switch(currentMode) {
            case MODE_REMOTE:
                break;
            case MODE_AVOID:
                ObAv();
                break;
            case MODE_TRACE:
                if (USART2_GetFrame(FrameBuf)) {
                    ParseFrame(FrameBuf);
                }
                break;
        }
        
        Delay_ms(10);
    }
}

// ================== 蓝牙中断 ==================
void USART1_IRQHandler(void) {
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
        uint8_t ch = USART_ReceiveData(USART1);
        
        if(currentMode == MODE_REMOTE) {
            switch(ch) {
                case 0x30: Servo_SetAngle(90); Car_Stop(); DisplayCurrentAction("Stop"); Serial_SendString("ACK: Stop\r\n"); break;
                case 0x31: Servo_SetAngle(90); Go_Ahead(); DisplayCurrentAction("Forward"); Serial_SendString("ACK: Forward\r\n"); break;
                case 0x32: Servo_SetAngle(90); Go_Back(); DisplayCurrentAction("Backward"); Serial_SendString("ACK: Backward\r\n"); break;
                case 0x33: Servo_SetAngle(180); Turn_Left(); DisplayCurrentAction("Left Turn"); Serial_SendString("ACK: Left Turn\r\n"); break;
                case 0x34: Servo_SetAngle(0);  Turn_Right(); DisplayCurrentAction("Right Turn"); Serial_SendString("ACK: Right Turn\r\n"); break;
                case 0x37: Servo_SetAngle(0);  Serial_SendString("ACK: Servo 0deg\r\n"); break;
                case 0x38: Servo_SetAngle(90); Serial_SendString("ACK: Servo 90deg\r\n"); break;
                case 0x39: Servo_SetAngle(180); Serial_SendString("ACK: Servo 180deg\r\n"); break;
                default:   DisplayCurrentAction("Unknown Cmd"); Serial_SendString("ERR: Unknown cmd\r\n"); break;
            }
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// ================== 语音输入中断 ==================

// PB5 → 遥控模式
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
        SwitchMode(MODE_REMOTE, "Voice");
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
}

// PB14 / PB15
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line14) != RESET)
    {
        SwitchMode(MODE_AVOID, "Voice");
        EXTI_ClearITPendingBit(EXTI_Line14);
    }

    if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        SwitchMode(MODE_TRACE, "Voice");
        EXTI_ClearITPendingBit(EXTI_Line15);
    }
}
