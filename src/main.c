/*
 *	@file		main.c
 *	@author		Minh Nguyen
 */

/******************************************************************************
 *	Includes
 *****************************************************************************/
#include <stddef.h>
#include <stdio.h>
#include "stm32l1xx.h"
#include "SetSysClock.h"

/******************************************************************************
 *	Typedef
 *****************************************************************************/

/******************************************************************************
 *	Define
 *****************************************************************************/

/******************************************************************************
 *	Macro
 *****************************************************************************/

/******************************************************************************
 *	Global variables
 *****************************************************************************/

/******************************************************************************
 *	Function prototypes
 *****************************************************************************/
void USART2_init(void);
void USART2_write(char data);
void delay_ms(unsigned long delay);
void read_dht22(int *hum, int *temp);
void delay_us(unsigned long delay);
void delay_Us(int delay);

/******************************************************************************
 *	Functions
 *****************************************************************************/

int main(void)
{
	/* Configure the system clock to 32 MHz and update SystemCoreClock */
	SetSysClock();
	SystemCoreClockUpdate();
	/* TODO - Add your application code here */
	int hum=0;
	int temp=0;
	char buf[20]="";

	RCC->AHBENR|=1; 		//Enable GPIOA ABH bus clock
	GPIOA->MODER|=0x400;	//GPIOA pin 5 to output
	USART2_init();

	while (1)
	{
		  read_dht22(&hum,&temp);					//read humidity and temperature
		  sprintf(buf,"temperature: %d \n\r",temp);	//print temperature
		  USART2_writeString(buf);
		  sprintf(buf,"humidity: %d \n\r",hum);		//print humidity
		  USART2_writeString(buf);
	}
	return 0;
}

void USART2_init(void)
{
	RCC->APB1ENR|=0x00020000; 	//Enable USART2 clock
	RCC->AHBENR|=0x00000001; 	//Enable GPIOA AHB clock
	GPIOA->AFR[0]=0x00000700;	//PA2 to AF7: USART2 TX
	GPIOA->AFR[0]|=0x00007000;	//PA3 to AF7: USART2 RX
	GPIOA->MODER|=0x00000020; 	//PA2 to alternative function mode
	GPIOA->MODER|=0x00000080; 	//PA3 to alternative function mode

	USART2->BRR = 0x00000115;	//115200 BAUD at clock 32MHz
	USART2->CR1 = 0x00000008;	//TE bit. Enable transmit
	USART2->CR1 |= 0x00000004;	//RE bit. Enable receiver
	USART2->CR1 |= 0x00002000;	//UE bit. Enable USART
}

void USART2_write(char data)
{
	while(!(USART2->SR&0x0080)){}	//wait while TX buffer is empty
		USART2->DR=(data);
}

void USART2_writeString(char *data)
{
	for(int i = 0; data[i] != '\0'; i++)
	{
		USART2_write(data[i]);
	}
}

void delay_ms(unsigned long delay)
{
	unsigned long i=0;
	SysTick->LOAD=32000-1; 	//32 000 000 = 1s so 32 000 = 1 ms
	SysTick->VAL=0;
	SysTick->CTRL=5; 		//enable counter

	while(i<delay)
	{
		//CTRL register bit 16 returns 1 if timer counted to 0 since last time this was read.
		while(!((SysTick->CTRL)&0x10000)){}
		i++;
	}
}

void delay_us(unsigned long delay)
{
	unsigned long i=0;
	SysTick->LOAD=32-1; 	//32 000 000 = 1s so 32 = 1 us
	SysTick->VAL=0;
	SysTick->CTRL=5;		//enable counter

	while(i<delay)
	{
		//CTRL register bit 16 returns 1 if timer counted to 0 since last time this was read.
		while(!((SysTick->CTRL)&0x10000)){}
		i++;
	}
}

void read_dht22(int *hum, int *temp)
{
	unsigned int humidity=0, i=0,temperature=0;
	unsigned int mask=0x80000000;
	RCC->AHBENR|=1; 		//Enable GPIOA ABH bus clock
	GPIOA->MODER|=0x1000; 	//GPIOA pin 6 to output
	GPIOA->ODR|=0x40; 		//high-state
	delay_ms(10);
	GPIOA->ODR&=~0x40; 		//low-state at least 500 us
	delay_ms(1);
	GPIOA->ODR|=0x40; 		//pin 6 high state and sensor gives this 20us-40us
	GPIOA->MODER&=~0x3000; 	//GPIOA pin 6 to input

	//response from sensor
	while((GPIOA->IDR & 0x40)){}
	while(!(GPIOA->IDR & 0x40)){}
	while((GPIOA->IDR & 0x40)){}

	//read values from sensor
	while(i<32)
	{
		while(!(GPIOA->IDR & 0x40)){}

		delay_us(35);

		if((GPIOA->IDR & 0x40)&&i<16)
		{
			humidity=humidity|(mask>>16);
		}
		if((GPIOA->IDR & 0x40)&&i>=16)
		{
			temperature=temperature|mask;
		}
		mask=(mask>>1);
		i++;

		while((GPIOA->IDR & 0x40)){}
	}
	*hum=(int)humidity;
	*temp=(int)temperature;
}
