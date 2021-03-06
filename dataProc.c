#include "pico/multicore.h"
#include "pico/runtime.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"

#include "dataProc.h"
#include "sharedData.h"

#include "kissfft/kiss_fftr.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define HANDSHAKE_FLAG 0x1234

//N.B. These buffers are never free'd, we reuse the memory for the entire
//program lifespan
static kiss_fft_cpx** ffts;
static float** channelBufs;
static size_t s_nSamples;
static unsigned int s_nChannels;
static unsigned int s_samplePeriodus;
static kiss_fftr_cfg fft_cfg;

void setupDataProc(size_t nSamples, unsigned int nChannels, unsigned int samplePeriodus)
{
	//Pre-allocate buffers for the de-interlaced channel data and
	//ffts of each channel
	channelBufs = (float**)(malloc(nChannels * sizeof(float*)));
	ffts = (kiss_fft_cpx**)(malloc(nChannels * sizeof(kiss_fft_cpx*)));
	for(unsigned int i = 0; i < nChannels; i++)
	{
		channelBufs[i] = (float*)(malloc(nSamples * sizeof(float)));
		ffts[i] = (kiss_fft_cpx*)(malloc(((nSamples/2)+1) * sizeof(kiss_fft_cpx)));
	}
	fft_cfg = kiss_fftr_alloc(nSamples, false, NULL, NULL);
	s_nSamples = nSamples;
	s_nChannels = nChannels;
	s_samplePeriodus = samplePeriodus;
}

void processDataThread()
{
	multicore_fifo_push_blocking(HANDSHAKE_FLAG);
	while(true)
	{
		//Busy loop, could also use an interrupt, but we don't care about power
		while(!newData);
			//printf("Running proc loop\n");
		size_t bufSize = s_nSamples * s_nChannels;
		for(int i = 0; i < bufSize; i++)
		{
			//TODO if nChannels = 1, we can skip the mod
			channelBufs[i%s_nChannels][i/s_nChannels] = validBuffer[i];
		}
		//Clear the flag, now that we've copied the data
		newData = false;

		//do the FFTs
		for(int i = 0; i < s_nChannels; i++)
		{
			kiss_fftr(fft_cfg, channelBufs[i], ffts[i]);
		}
		for(int i = 0; i < s_nChannels; i++)
		{
			reportTimeDomain(i);
			reportPower(i);
		}
	}
}

void reportPower(unsigned int channel)
{
	printf("T_us=%llu;c=%d;f;n=%d;Ts_us=%u;", validChunkTimestamp_us, 
			channel, s_nSamples, s_samplePeriodus);
	for(unsigned int i = 0; i < (s_nSamples/2)+1; i++)
	{
		printf("%f", ffts[channel][i].r*ffts[channel][i].r + 
				ffts[channel][i].i * ffts[channel][i].i);
		if(i != (s_nSamples/2))
		{
			printf(",");
		}
	}
	printf("\n");
}

void reportTimeDomain(unsigned int channel)
{
	printf("T_us=%llu;c=%d;t;n=%d;Ts_us=%u;", validChunkTimestamp_us, 
			channel, s_nSamples, s_samplePeriodus);
	printf("%f",validBuffer[0]);
	for(unsigned int i = 1; i < s_nSamples; i++)
	{
		printf(",%f", validBuffer[i]);
	}
	printf("\n");
}
