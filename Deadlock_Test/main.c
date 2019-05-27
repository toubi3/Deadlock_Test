/*
 * Deadlock_Test.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

SemaphoreHandle_t data1Key; //Lock for data1
uint32_t data1 = 0;			//P-Resource data1

SemaphoreHandle_t data2Key; //Lock for data2
uint32_t data2 = 0;			//P-Resource data2

extern void vApplicationIdleHook( void );

void vT1(void *pvParameters);
void vT2(void *pvParameters);

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	data1Key = xSemaphoreCreateMutex(); //Create Lock
	data2Key = xSemaphoreCreateMutex(); //Create Lock

	PORTF.DIRSET = PIN0_bm | PIN1_bm;

	xTaskCreate( vT1, (const char *) "T1", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);
	xTaskCreate( vT2, (const char *) "T2", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);

	vDisplayClear();
	vTaskStartScheduler();
	return 0;
}

void vT1(void *pvParameters) {	
	uint32_t cTaskCounter = 0;
	for(;;) {
		if(xSemaphoreTake(data2Key, 5 / portTICK_RATE_MS)) { //Lock P-Resource data2			
			vTaskDelay(101 / portTICK_RATE_MS);
			PORTF.OUTCLR = 0x01;
			cTaskCounter++;
			data2 = cTaskCounter;
			if(xSemaphoreTake(data1Key, 5 / portTICK_RATE_MS)) { //Lock P-Resource data1
				vDisplayWriteStringAtPos(0,0,"Task1Counter: %d", cTaskCounter);
				vDisplayWriteStringAtPos(1,0,"d1: %d / ", data1);
				vDisplayWriteStringAtPos(1,12, "d2:%d", data2);
			}
			xSemaphoreGive(data1Key); //Unlock data1
			xSemaphoreGive(data2Key); //Unlock data2
			PORTF.OUTSET = 0x01;
		}
	}
}

void vT2(void *pvParameters) {
	uint32_t dTaskCounter = 0;
	for(;;) {
		if(xSemaphoreTake(data1Key, 5 / portTICK_RATE_MS)) { //Lock Resource data1
			vTaskDelay(100 / portTICK_RATE_MS);		
			PORTF.OUTCLR = 0x02;
			dTaskCounter++;
			data1 = dTaskCounter;
			if(xSemaphoreTake(data2Key, 5 / portTICK_RATE_MS)) { //Lock Resource data2
				vDisplayWriteStringAtPos(2,0,"Task2Counter: %d", dTaskCounter);
				vDisplayWriteStringAtPos(3,0,"d1: %d / ", data1);
				vDisplayWriteStringAtPos(3,12, "d2:%d", data2);
			}
			xSemaphoreGive(data1Key); //Unlock data1
			xSemaphoreGive(data2Key); //Unlock data2
			PORTF.OUTSET = 0x02;
		}
	}
}