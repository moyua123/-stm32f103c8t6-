#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Key.h"
#include "CarControl.h"
#include "Serial.h"
#include <string.h>
#include "Servo.h"
#include "Ultrasound.h"
#include "infrared.h"
#include "TraceLine_PID.h"  // 引入 PID 循迹功能

#define RX_BUFFER_SIZE 64

// 红外传感器引脚定义
#define ll GPIO_Pin_0
#define l  GPIO_Pin_1
#define r  GPIO_Pin_2
#define rr GPIO_Pin_3
#define Infrared(GPIOX,Pin) GPIO_ReadInputDataBit(GPIOX, Pin)

// 工作模式枚举
typedef enum {
    MODE_REMOTE,     // 遥控模式
    MODE_AVOID,      // 避障模式
    MODE_TRACE,      // 循迹模式
    MODE_COUNT       // 模式总数（用于循环）
} WorkMode;

// 全局变量
uint16_t KeyNum;
WorkMode currentMode = MODE_REMOTE; // 默认遥控模式
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_index = 0;

// 避障相关变量
uint16_t dis = 0;
uint16_t disr = 0;
uint16_t disl = 0;

// 函数声明
void DisplaySystemStatus(void);
void DisplayCurrentAction(char* action);
void ObAv(void);

// 显示系统状态
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

// 显示当前动作
void DisplayCurrentAction(char* action) {
    OLED_ShowString(3, 9, action);
}

// 避障功能
void ObAv(void) {
    Go_Ahead();
    dis = Test_Distance();
    Serial_SendNumber(dis, 3);
    
    if(dis < 30) {
        Car_Stop();
        Servo_SetAngle(0);
        Delay_ms(1000);
        disr = Test_Distance();
        Serial_SendNumber(disr, 3);
        
        if(disr > 30) {
            Servo_SetAngle(90);
            Delay_ms(1000);
            Turn_Right();
            Delay_ms(1000);
            Go_Ahead();
        } else {
            Servo_SetAngle(180);
            Delay_ms(1000);
            disl = Test_Distance();
            Serial_SendNumber(disl, 3);
            
            if(disl > 30) {
                Servo_SetAngle(90);
                Delay_ms(1000);
                Turn_Left();
                Delay_ms(1000);
                Go_Ahead();
            } else {
                Servo_SetAngle(90);
                Go_Back();
                Delay_ms(1000);
            }
        }
    }
}

int main(void) {
    /* 模块初始化 */
    Car_Init();
    OLED_Init();
    Key_Init();
    Serial_Init();
    Servo_Init();
    Ultrasound_Init();
    Infrared_Init();
    
    /* 初始显示 */
    DisplaySystemStatus();
    DisplayCurrentAction("Stop");
    Car_Stop();
    
    /* 发送就绪消息 */
    Serial_SendString("SmartCar Ready!\r\n");

    while (1) {
        KeyNum = Key_GetNum();
        
        // 按键切换模式
        if(KeyNum == 1) {
            currentMode = (WorkMode)((currentMode + 1) % MODE_COUNT);
            
            Car_Stop();
            Servo_SetAngle(90);
            DisplaySystemStatus();
            
            switch(currentMode) {
                case MODE_REMOTE:
                    DisplayCurrentAction("Stop");
                    Serial_SendString("Mode: Remote\r\n");
                    break;
                case MODE_AVOID:
                    DisplayCurrentAction("Avoiding");
                    Serial_SendString("Mode: Avoid\r\n");
                    break;
                case MODE_TRACE:
                    DisplayCurrentAction("Tracing");
                    Serial_SendString("Mode: Trace\r\n");
                    break;
                default:
                    DisplayCurrentAction("Unknown");
                    Serial_SendString("Mode: Unknown\r\n");
            }
        }
        
        // 执行对应模式功能
        switch(currentMode) {
            case MODE_REMOTE:
                // 遥控模式由蓝牙中断处理
                break;
            case MODE_AVOID:
                ObAv();
                break;
            case MODE_TRACE:
                TraceLine_PID();  // PID循迹功能调用
                break;
        }
        
        Delay_ms(10); // 防止过于频繁检测
    }
}

// 蓝牙中断处理
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
