#ifndef __I2C_H
#define __I2C_H

void I2C1_Init(void); // 重命名函数
uint8_t I2C_Scan_8574(void);
uint8_t LineSensor_ReadByte(uint8_t addr7);
#endif
