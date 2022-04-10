#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include <stdio.h>

#include "sharedData.h"
#include "dataRecv.h"

#define LED_PIN  PICO_DEFAULT_LED_PIN
int main()
{
	stdio_init_all();
	printf("Starting Program!\n");

	initDataRecvr(100,1, 1365);
	recvThreadMain();
	for(;;)
		printf("Running\n");
}
