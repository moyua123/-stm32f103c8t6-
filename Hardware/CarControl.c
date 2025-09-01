#include "stm32f10x.h"                  // Device header
#include "Motor.h"
#include "Delay.h"

void Car_Init(void)
{
	Motor_Init();
}

void Go_Ahead(void)
{
	Motor_SetLeftSpeed(-90);
	Motor_SetRightSpeed(-90);
}

void Go_Back(void)
{
	Motor_SetLeftSpeed(90);
	Motor_SetRightSpeed(90);
}

void Turn_Left(void)
{
	Motor_SetLeftSpeed(0);
	Motor_SetRightSpeed(-90);
}

void Turn_Right(void)
{
	Motor_SetLeftSpeed(-90);
	Motor_SetRightSpeed(0);
}
void Self_Left(void)
{
	Motor_SetLeftSpeed(90);
	Motor_SetRightSpeed(-90);
}

void Self_Right(void)
{
	Motor_SetLeftSpeed(-90);
	Motor_SetRightSpeed(90);
}
void Car_Stop(void)
{
	Motor_SetLeftSpeed(0);
	Motor_SetRightSpeed(0);
}
