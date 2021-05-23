#pragma once
#include "stm32f0xx.h"
#include "renderAPI.h"
#include <math.h>


#define b1111 15
#define b11 3
#define b111 7
#define b101 5
#define b110 6
#define b11111111 255

static volatile uint32_t rawValue = 0;
int group = 1;
static volatile bool _debug = true;

static volatile uint32_t raw_result = 0;

typedef struct{
uint32_t   s[3];
uint8_t      i;
uint8_t      ready;
}TSC_RESULT;

TSC_RESULT Result;

void ResetSensors(TSC_RESULT *pResult)
{
	for(uint8_t i=0;i<3; i++) 
		pResult->s[i] = 0xFFFF;

	pResult->i = 0;
	pResult->ready=0;
}

void ReadSensors(TSC_RESULT *pResult)
{
	raw_result =  (Result.s[0] + Result.s[1] + Result.s[2])/3;

	if(_debug)
	{
		drawSpiPos(0, Result.s[0]%8);
		drawSpiPos(1, Result.s[1]%8);
		drawSpiPos(2, Result.s[2]%8);
		drawSpiPos(4, raw_result%8);
	}
	
	ResetSensors(pResult); 
	TSC->IOGCSR &= ~b11111111;
	TSC->IOGCSR |= TSC_IOGCSR_G1E; 
	TSC->IOCCR &= ~b1111; 
	TSC->IOCCR |= TSC_IOCCR_G1_IO1;
	__enable_irq ();
	
	NVIC_EnableIRQ(TSC_IRQn);//bind interruput
	TSC->CR |= TSC_CR_START;
	
}

void TSC_IRQHandler(void)
{
	if (TSC -> ISR & TSC_ISR_MCEF) { 
    TSC -> ICR |= (TSC_ICR_EOAIC | TSC_ICR_MCEIC);
  
    Result.s[0] = Result.s[1] = Result.s[2] = 0;
    Result.i = 0;
    Result.ready = 1;
   
		drawSpiPos(6,6);
    return;
  }
	
  TSC -> ICR |= (TSC_ICR_EOAIC);
  switch (Result.i)
	{
  case 0: 
    Result.s[0] = TSC -> IOGXCR[0];
    TSC -> IOCCR &= ~b1111; 
    TSC -> IOCCR |= TSC_IOCCR_G1_IO2;
    break;
  case 1:
    Result.s[1] = TSC -> IOGXCR[0];
    TSC -> IOCCR &= ~b1111;
    TSC -> IOCCR |= TSC_IOCCR_G1_IO3;
    break;
  case 2:
    Result.s[2] = TSC -> IOGXCR[0];
    TSC -> IOCCR &= ~b1111;
    break;
  }
  Result.i++;
  if (Result.i < 3) {
    TSC -> CR |= TSC_CR_START;
    Result.ready = 0;
  } else {
    Result.ready = 1;
  }	
}

void lateTSCinit()
{
	TSC->IER |= TSC_IER_EOAIE | TSC_IER_MCEIE; //enable end of acquisition interrupt
	
	NVIC_EnableIRQ(TSC_IRQn);//bind interruput
	
	//TSC->CR |= TSC_CR_PGPSC_2 | TSC_CR_PGPSC_0 | TSC_CR_CTPH_0 | TSC_CR_CTPL_0 | TSC_CR_MCV_2 | TSC_CR_MCV_1 | TSC_CR_TSCE;//TSC_CR_PGPSC_2 | TSC_CR_PGPSC_0 | TSC_CR_CTPH_0 | TSC_CR_CTPL_0 | TSC_CR_MCV_2 | TSC_CR_MCV_1 | TSC_CR_TSCE;//pure magic lol
	TSC->CR = TSC_CR_CTPH_0   // Charge transfer pulse high [3:0] = t_PGCLK * "?32"
      | TSC_CR_CTPL_0        // Charge transfer pulse low [3:0] = t_PGCLK * "x1"
      | (0 << TSC_CR_SSD_Pos)            // Spread spectrum deviation [6:0] = "x1" * t_SSCLK
      | (0 << TSC_CR_SSE_Pos)            // Spread spectrum enable = 0: DISABLE / 1: ENABLE
      | (1 << TSC_CR_SSPSC_Pos)         // Spread spectrum prescaler = 0: f_HCLK / 1: 0.5 * f_HCLK
      | (b101 << TSC_CR_PGPSC_Pos)   // Pulse generator prescaler [2:0] (f_HCLK/1... f_HCLK/128), t_PGCLK = b101 = /32
      | (b110 << TSC_CR_MCV_Pos)      // Max count value [2:0] = datasheet RM0091 -> p.311/1004, b110 = 16383
      | (0 << TSC_CR_IODEF_Pos)         // I/O Default mode: 0: output push-pull LOW / 1: input floating
      | (0 << TSC_CR_SYNCPOL_Pos)      // Synchronization pin polarity
      | (0 << TSC_CR_AM_Pos)            // Acquisition mode: 0: Normal acquisition mode / 1: Synchronized acquisition mode
      | (0 << TSC_CR_START_Pos)         // Start a new acquisition: 0 (hard): Acquisition not started / 1 (soft): Start a new acquisition
      | TSC_CR_TSCE;         // Touch sensing controller enable: 0: disabled / 1: enabled
	//TSC->CR |= TSC_CR_START;//(0x01 + 0x00C0 + 0x02); //enable TSC and start acquisition
	//TSC->CR |= TSC_CR_AM;

}
//GPIOA->AFR[0] |= 3 << (0) * 4;
//alt.func. number ^;pin.^;   ^ - always the same
void touch_init(void) 
{
	//PA7  PA6
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	
	GPIOA->MODER &= ~(GPIO_MODER_MODER0_Msk | GPIO_MODER_MODER1_Msk | GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER3_Msk);
	GPIOA->MODER |=  GPIO_MODER_MODER0_1 | GPIO_MODER_MODER1_1 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
	
	
	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL0 | GPIO_AFRL_AFSEL1 | GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
	GPIOA->AFR[0] |= (b11 << GPIO_AFRL_AFSEL0_Pos) | (b11 << GPIO_AFRL_AFSEL1_Pos) | (b11 << GPIO_AFRL_AFSEL2_Pos) | (b11 << GPIO_AFRL_AFSEL3_Pos);

	//GPIOA->AFR[0] |= (3 << (0) * 4) + (3 << (1) * 4) + (3 << (2) * 4) + (3 << (3) * 4);
	
	RCC->AHBENR |= RCC_AHBENR_TSEN;//enable TS clock
		
	TSC->IOGCSR |= TSC_IOGCSR_G1E; //enable G2 analog group
	
	TSC->IOHCR &= (uint32_t)(~(TSC_IOHCR_G1_IO4 | TSC_IOHCR_G1_IO3 | TSC_IOHCR_G1_IO2 | TSC_IOHCR_G1_IO1)); //disable hysteresis on PA6 and PA7
	
	GPIOA->OTYPER |= (GPIO_OTYPER_OT_3); // OD
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT_0 | GPIO_OTYPER_OT_1 | GPIO_OTYPER_OT_2); // PP
	
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR0 | GPIO_PUPDR_PUPDR1 | GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3);

	GPIOA->OSPEEDR |= (b11 << GPIO_OSPEEDR_OSPEEDR0_Pos) | (b11 << GPIO_OSPEEDR_OSPEEDR1_Pos) | (b11 << GPIO_OSPEEDR_OSPEEDR2_Pos) | (b11 << GPIO_OSPEEDR_OSPEEDR3_Pos);

	lateTSCinit();
	TSC->IOSCR |= TSC_IOSCR_G1_IO4; //enable G1_IO4 as sampling capacitor

}