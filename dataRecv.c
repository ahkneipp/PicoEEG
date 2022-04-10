#include "pico/runtime.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include <stdlib.h>

#include "dataRecv.h"
#include "sharedData.h"

#include <stdbool.h>
#include <stdio.h>

#define PICO_ADC_0_PIN 26
#define ADC_FIFO_IRQ 22

#define ADC_FS 3.3

static float* buf1;
static float* buf2;
static float* currentBuf;
static unsigned int bufIdx = 0;
static size_t bufLen;

volatile float* validBuf;
volatile bool newData;
static volatile bool fifoReady = false;

static bool initialized = false;
void adcFifoISR(void);

/*
 * Random notes:
 * Probably not interested in frequencies > 59Hz
 * -> Sampling Freq >= 118 Hz -> 120Hz for cleanliness
 * Ts,max = 8.33ms -> many many clk cycles
 * Very memory limited rather than CPU limited for this program, so use your CPU
 *   to save memory
 */

void initDataRecvr(int nSamples, int nChannels, int sampleRate)
{
	//TODO check if memory use is acceptable
	bufLen = nSamples * nChannels;
	buf1 = (float*)malloc(sizeof(float) * bufLen);
	buf2 = (float*)malloc(sizeof(float) * bufLen);
	currentBuf = buf1;
	bufIdx = 0;
	newData = false;
	validBuf = NULL;
	setupADC(nChannels, sampleRate);
}

void setupADC(int nChannels, int samplePeriodus)
{
	//TODO clkdiv
	//TODO validate nChannels:  1 <= nChannels <= 3
	adc_init();
	//Since we know the GPIO pin # for adc input 0, and all of the
	//ADC pins are sequential, we can loop like this
	for(int pin = PICO_ADC_0_PIN; pin < PICO_ADC_0_PIN + nChannels; pin++)
	{
		adc_gpio_init(pin);
	}
	if(nChannels == 1)
	{
		adc_select_input(0);
	}
	else
	{
		adc_set_round_robin((unsigned char)(0x07) >> 3-nChannels);
	}
	//Enable the fifo
	//disable DMA requests
	//Send an IRQ when there are nChannels samples in the buffer
	//bit 15 does not contains the error flag
	//Don't do a byte shift of the ADC result
	adc_fifo_setup(true, false, nChannels, false, false);
	//Set an IRQ handler for when the FIFO fills up
	irq_set_exclusive_handler(ADC_FIFO_IRQ, adcFifoISR);
	//Enable the FIFO interrupt at the FIFO
	adc_irq_set_enabled(true);
	//Enable the FIFO interrupt at the CPU
	irq_set_enabled(ADC_FIFO_IRQ, true);
	//Set the sample rate
	//MAX VALUE 65535
	adc_set_clkdiv(48000000 * (samplePeriodus/1000000.0));
}

void recvThreadMain()
{
	adc_run(true);
	for(;;)
	{
		irq_set_enabled(ADC_FIFO_IRQ, true);
		//Busy loop
		while(!fifoReady); {printf("ADC DIV: %x\n", (unsigned int)(adc_hw->div>>8));}
		while(!adc_fifo_is_empty())
		{
			//TODO potential off-by-one
			// Store the received voltage in the current buffer
			currentBuf[bufIdx++] = adc_fifo_get() * (ADC_FS / (1<<12));
		}
		fifoReady = false;
		if(bufIdx >= bufLen)
		{
			currentBuf = currentBuf == buf2 ? buf1 : buf2;
			bufIdx = 0;
			//TODO critical section around newData?
			newData = true;
			printf("Swapped buffer\n");
		}
	}
}


void adcFifoISR(void)
{
	irq_set_enabled(ADC_FIFO_IRQ, false);
	printf("Ran ISR\n");
	//adc_run(false);
	fifoReady = true;
}
