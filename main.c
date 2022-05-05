#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "hardware/adc.h"

#include <stdio.h>

#include "sharedData.h"
#include "dataRecv.h"
#include "dataProc.h"

#define LED_PIN  PICO_DEFAULT_LED_PIN
int main()
{
	stdio_init_all();
	printf("Starting Program!\n");

	//Capture 10s of samples from channel 1 @ 100 Hz
	initDataRecvr(3000, 1, 10000);
	setupDataProc(3000, 1, 10000);

	multicore_reset_core1();
	multicore_launch_core1(&processDataThread);
	uint32_t v = multicore_fifo_pop_blocking();

	recvThreadMain();
	for(;;)
		printf("Running\n");
}
