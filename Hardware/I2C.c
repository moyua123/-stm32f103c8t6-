#include "stm32f10x.h"
#include "Delay.h"

// === 根据硬件情况选择 ===
#define INVERT_BITS   1   // 黑=低电平 → 取反让黑=1，白=0
#define FLIP_BITORDER 1   // P0 在最右 → 翻转位序使 bit0=最左

// --- 内部函数：读取1字节并NACK结束 ---
static uint8_t I2C_Read_NACK(void) {
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    uint8_t data = I2C_ReceiveData(I2C1);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return data;
}

static inline uint8_t flip8(uint8_t x) {
    x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
    return x;
}

// --- I2C 初始化 ---
void I2C1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_I2C1, DISABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1         = 0x08;
    I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed          = 100000;

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

// --- 地址扫描，返回找到的地址 ---
uint8_t I2C_Scan_8574(void) {
    for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
        I2C_GenerateSTART(I2C1, ENABLE);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Receiver);
        if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
            (void)I2C_Read_NACK();
            return addr;
        }
        I2C_GenerateSTOP(I2C1, ENABLE);
    }
    for (uint8_t addr = 0x38; addr <= 0x3F; addr++) {
        I2C_GenerateSTART(I2C1, ENABLE);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Receiver);
        if (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
            (void)I2C_Read_NACK();
            return addr;
        }
        I2C_GenerateSTOP(I2C1, ENABLE);
    }
    return 0xFF;
}

// --- 读取传感器值（带取反/翻转）---
uint8_t LineSensor_ReadByte(uint8_t addr7) {
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, addr7, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    uint8_t v = I2C_Read_NACK();

#if INVERT_BITS
    v = ~v;
#endif
#if FLIP_BITORDER
    v = flip8(v);
#endif
    return v;
}
