#include "pico/runtime.h"
#include "pico/time.h"
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
static unsigned int s_samplePeriod_us;
static unsigned int s_nSamples;
static unsigned int s_nChannels;

volatile unsigned long long int validChunkTimestamp_us = 0;
volatile float* validBuffer;
volatile bool newData = false;
static volatile bool fifoReady = false;

static bool initialized = false;
void adcFifoISR(void);
int64_t captureSampleCallback(alarm_id_t id, void* user_data);

/*
 * Random notes:
 * Probably not interested in frequencies > 59Hz
 * -> Sampling Freq >= 118 Hz -> 120Hz for cleanliness
 * Ts,max = 8.33ms -> many many clk cycles
 * Very memory limited rather than CPU limited for this program, so use your CPU
 *   to save memory
 */

void initDataRecvr(unsigned int nSamples, unsigned int nChannels, 
		unsigned int sampleRate)
{
	//TODO check if memory use is acceptable
	bufLen = nSamples * nChannels;
	buf1 = (float*)malloc(sizeof(float) * bufLen);
	buf2 = (float*)malloc(sizeof(float) * bufLen);
	currentBuf = buf1;
	bufIdx = 0;
	newData = false;
	validBuffer = NULL;
	s_samplePeriod_us = sampleRate;
	s_nSamples = nSamples;
	s_nChannels = nChannels;
	alarm_pool_init_default();
	// Wait 5 seconds before starting to collect samples so PC recv'r can start
	if(add_alarm_in_ms(5000, captureSampleCallback,(void*)sampleRate,true) == -1)
	{
		printf("Failed to initialize sample capture alarm\n");
	}
	setupADC(nChannels, sampleRate);
}

void setupADC(unsigned int nChannels, unsigned int samplePeriodus)
{
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
		adc_set_round_robin((unsigned char)(0x07) >> (3-nChannels));
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
}

void recvThreadMain()
{
	adc_run(true);
	for(;;)
	{
		irq_set_enabled(ADC_FIFO_IRQ, true);
		//Busy loop
		while(!fifoReady);
		for(int i = 0; i < s_nChannels; i++)
		{
			//TODO potential off-by-one
			// Store the received voltage in the current buffer
			currentBuf[bufIdx++] = adc_fifo_get() * (ADC_FS / (1<<12));
		}
		adc_fifo_drain();
		fifoReady = false;
		if(bufIdx >= bufLen)
		{
			validBuffer = currentBuf;
			//TODO first approx of a timestamp, 
			//may need to actually have a timer or use millis or something
			validChunkTimestamp_us += s_samplePeriod_us * s_nSamples;
			currentBuf = currentBuf == buf2 ? buf1 : buf2;
			bufIdx = 0;
			//TODO critical section around newData?
			newData = true;
			//printf("Swapped buffer\n");
		}
	}
}

int64_t captureSampleCallback(alarm_id_t id, void* user_data)
{
	unsigned int samplePeriod_us = (unsigned int)(user_data);
	adc_run(true);
	return -((int64_t)(samplePeriod_us));
}

void adcFifoISR(void)
{
	irq_set_enabled(ADC_FIFO_IRQ, false);
	//printf("Ran ISR\n");
	adc_run(false);
	fifoReady = true;
}
