#include <stm32f0xx.h>
#include <system_stm32f0xx.h>
#include <stdlib.h>

#include "buttons.h"
#include "renderAPI.h"
#include "pingPong.h"
#include "tscHandler.h"
#include "leds.h"
#include "usart_base.h"
#include "tim_timer.h"

#define global static volatile
typedef struct 
{
	uint32_t counter;
} Context;

void init(void);
void loop(Context*);
void resetAll(void);

//set and reset bit for volatile uint32_t
void setBitV(volatile uint32_t* bit, uint32_t value);
void resetBitV(volatile uint32_t* bit, uint32_t value);
void wait(uint32_t);
bool buttonDown(void);
void SysTick_Handler (void);
void traffic(void);
void SysTick_Init(void);
void SysTick_Wait(uint32_t);
void renderLoop(void);
void DMA1_Channel1_IRQHandler(void);

#define F_CPU 		72000000UL	

static volatile uint32_t timestamp = 0;
  
void SysTick_Handler (void)
{
   timestamp++;
	 fetchButtons(timestamp);	
	 //renderLoop(); 
}
static int ypos = 0;
static int xpos = 0;
static volatile bool use_dma = true;

void SPI2_IRQHandler(void)
{	
	volatile uint16_t data = SPI2->DR;
	renderInt();
	resetBitV(&LED, Rled);
}

void wait(uint32_t delay)
{
	uint32_t delaystamp = timestamp + delay;
	while(1)
	{
		 if (timestamp > delaystamp)
		 {
				break;
		 }
	}
}
void resetAll()
{
	for(unsigned long i =0;i<15;++i)
	{
		GPIOC->MODER |= GPIO_MODER_MODER0+i;
	}
}

void init(void)
{
	resetAll();
	
	//SystemInit(); // Device initialization
	SystemInit();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);
	SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
	
	touch_init();
	initBtns();
	resetBtns();
	initSPI();
	initPong();
	initializeTimer();
	
	initTransfer();
	
	setTransferMode(false);
}

static volatile uint16_t levels[8];

static int clamp(int val, int min,int max)
{
	if(val>max)
		return max;
	if(val<min)
		return min;
	return val;
}

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
static volatile int cntr = 0;
static volatile uint32_t lastSensorPoll = 0;
global int flippedC = 0;
global uint8_t transmitData = 4;
void loop(Context* context)
{
	if(buttonDown())
	{
		//states[0] = true;
	}
	
	_debug = false;
	onUpdatePong(timestamp);
	
	if (transfer.isTransmit)
	{
		
		if(flippedC == 9)
		{
			transfer.data = (uint8_t)63;
			transmitMessage();
		}
		if(flippedC==10)
		{
			flippedC = 0;
			setTransferMode(false);
			wait(50);
			receiveMessage();
			wait(50);
			receiveMessage();
		}
		else
		{
			transfer.data = (uint8_t)cursorY;
			transmitMessage();
		}
		flippedC++;
	}
	else
	{
		receiveMessage();
		if(transfer.data==63)
		{
			setTransferMode(true);
			wait(100);
		}
		else
			drawSpiPos(transfer.data, 0);
		if(flippedC>10)
		{
			//flippedC = 0;
			//setTransferMode(true);
		}
	}
	//flippedC++;
	//if(timestamp - lastSensorPoll > 100)
	//{
	//		ReadSensors(&Result);
	//		lastSensorPoll = timestamp;
	//}
	
	clientFlush();
	clearImage();
	
	wait(100);
}	

int main(void)
{
	Context* context = (Context*)malloc(sizeof(Context));
	context->counter = 0;
	
	init();
	
	while(1)
	{
		loop(context);
	}
}
