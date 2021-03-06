#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stm32f0xx.h>
#include "renderAPI.h"
#include <string.h>

typedef struct 
{
	int rows[8];
} GLRenderBuffer;

static volatile GLRenderBuffer buffers[2];
static volatile GLRenderBuffer* frontBuffer;
static volatile GLRenderBuffer* backBuffer;

static void initSPI()
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; 
	
	// Set Alternate function 1 to pin 
	GPIOB->AFR[1] |= 0 << (15 - 8) * 4;
	// Set alternate func 1 (AF0) to pin 13
	GPIOB->AFR[1] |= 0 << (13 - 8) * 4;
	
	SPI2->CR1 = 
		  SPI_CR1_SSM 
		| SPI_CR1_SSI 
		//| SPI_CR1_LSBFIRST 
		| SPI_CR1_BR 
		| SPI_CR1_MSTR
		//| SPI_CR1_BIDIOE
		//| SPI_CR1_BIDIMODE
		| SPI_CR1_CPOL
		| SPI_CR1_CPHA;
	
	SPI2->CR2 = SPI_CR2_DS;
	
	GPIOB->MODER |= GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1;
	GPIOA->MODER |= GPIO_MODER_MODER8_0;
	
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR13_1 | GPIO_PUPDR_PUPDR15_1;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR8_1;
	
	SPI2->CR1 |= SPI_CR1_SPE;
	
	
	//Enable Rx buffer not empty interrupt
	SET_BIT(SPI2->CR2, SPI_CR2_RXNEIE);
	NVIC_EnableIRQ(SPI2_IRQn);

	SPI2->DR = 0;
	frontBuffer = &buffers[0];
	backBuffer = &buffers[1];

  //Enable Tx buffer empty interrupt
  //SET_BIT(SPI2->CR2, SPI_CR2_TXEIE);
  //Enable error interrupt
  //SET_BIT(SPI2->CR2, SPI_CR2_ERRIE);
}

void drawRow(int row, volatile GLRenderBuffer* renderBuffer)
{
	int result = 0;

	result |= renderBuffer->rows[row];
	
	SPI2->DR |= result;
}

static void drawSpiPos(int x, int y)
{
	if(y>=8 || y<0 || x<0 || x>=8)
		return;
		
	frontBuffer->rows[y] |= (0x1U << y) << 8 | (0x1U << x);
}
bool renderBusy()
{
	return SPI2->SR & SPI_SR_BSY;
}
void renderBegin()
{
	GPIOA->ODR &= ~GPIO_ODR_8;
}
void renderFlush()
{
	GPIOA->ODR |= GPIO_ODR_8;
}
static void clearImage()
{
	memset((void*)frontBuffer, 0, sizeof(GLRenderBuffer));
}

static volatile bool renderedImage = true;
static volatile int renderingLine = 0;
bool flushed = false;


void renderInt(void)
{
	if(renderingLine >= 8)
		renderedImage = true;
	if(renderedImage)
	{
		if(!flushed)
			return;
		renderingLine = 0;
		renderedImage = false;
	}
	
	renderFlush();
	
	renderBegin();
	drawRow(renderingLine, backBuffer);
	renderingLine++;
	
}
void clientFlush()
{
	flushed = true;
	volatile GLRenderBuffer* tmp = frontBuffer;
	frontBuffer = backBuffer;
	backBuffer = tmp;
}