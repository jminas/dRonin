/**
******************************************************************************
* @addtogroup TauLabsTargets Tau Labs Targets
* @{
* @addtogroup FlyingF4 FlyingF4 support files
* @{
*
* @file       main.c
* @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
*             AJ Christensen <aj@junglistheavy.industries>
* @brief      Start ChiBiOS/RT and the Modules.
* @see        The GNU Public License (GPL) Version 3
*
*****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/* OpenPilot Includes */
#include "openpilot.h"
#include "uavobjectsinit.h"
#include "systemmod.h"
#include "pios_thread.h"
#include "hwbrainre1.h"

#include "misc_math.h"
#include "fpga_drv.h"

#if defined(PIOS_INCLUDE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif /* defined(PIOS_INCLUDE_FREERTOS) */

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);
void ledUpdatePeridodicCb(UAVObjEvent * ev, void *ctx, void *obj, int len);


/* Local Variables */
#define INIT_TASK_PRIORITY  PIOS_THREAD_PRIO_HIGHEST
#define INIT_TASK_STACK   1024
static struct pios_thread *initTaskHandle;


/* Function Prototypes */
static void initTask(void *parameters);

/**
 * Tau Labs Main function:
 *
 * Initialize PiOS<BR>
 * Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
 * Start FreeRTOS Scheduler (vTaskStartScheduler)<BR>
 * If something goes wrong, blink LED1 and LED2 every 100ms
 *
 */
int main()
{
	/* NOTE: Do NOT modify the following start-up sequence */
	/* Any new initialization functions should be added in OpenPilotInit() */
	PIOS_heap_initialize_blocks();

#if defined(PIOS_INCLUDE_CHIBIOS)
	halInit();
	chSysInit();

	boardInit();
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

	/* For Revolution we use a FreeRTOS task to bring up the system so we can */
	/* always rely on FreeRTOS primitive */
	initTaskHandle = PIOS_Thread_Create(initTask, "init", INIT_TASK_STACK, NULL, INIT_TASK_PRIORITY);
	PIOS_Assert(initTaskHandle != NULL);

#if defined(PIOS_INCLUDE_FREERTOS)
	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be running. */
	/* Do some PIOS_ANNUNC_HEARTBEAT to user that something bad just happened */
	PIOS_ANNUNC_Off(PIOS_ANNUNC_HEARTBEAT); \
	for(;;) { \
		PIOS_ANNUNC_Toggle(PIOS_ANNUNC_HEARTBEAT); \
		PIOS_DELAY_WaitmS(100); \
	};
#elif defined(PIOS_INCLUDE_CHIBIOS)
	PIOS_Thread_Sleep(PIOS_THREAD_TIMEOUT_MAX);
#endif /* defined(PIOS_INCLUDE_CHIBIOS) */

	return 0;
}

#define STHINGS_TASK_PRIORITY	PIOS_THREAD_PRIO_NORMAL
#define STHINGS_TASK_STACK		512
static void strangerThingsTask(void *parameters);

/**
 * Initialization task.
 *
 * Runs board and module initialization, then terminates.
 */
void initTask(void *parameters)
{
	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

//	/* Schedule a periodic callback to update the LEDs */
//	UAVObjEvent ev = {
//		.obj = HwBrainRE1Handle(),
//		.instId = 0,
//		.event = 0,
//	};
//	EventPeriodicCallbackCreate(&ev, ledUpdatePeridodicCb, 31);

	PIOS_Thread_Create(strangerThingsTask, "strangerThings", STHINGS_TASK_STACK, NULL, STHINGS_TASK_PRIORITY);

	/* terminate this task */
	PIOS_Thread_Delete(NULL);
}


#define LETTER_DURATIOM_MIN 500
#define LETTER_DURATIOM_MAX 1000

#define WORD_PAUSE_MIN 2000
#define WORD_PAUSE_MAX 5000

#define NUMLEDS 26
#define NUMLEDBYTES (3 * NUMLEDS)
#define NUMCOLORS 8
const uint8_t COLORS[NUMCOLORS][3] = {{255, 255, 255}, {255, 0,   0}, {255, 69,   0}, {255, 255, 0}, {0, 255, 0}, {0,   255, 255}, {0,   0,   255}, {255, 0,   255}};

uint8_t colors[NUMLEDS];


uint8_t LEDBUFF[NUMLEDBYTES];
const uint8_t LED_ORDER[NUMLEDS] = { 17, 18, 19, 20, 21, 22, 23, 24, 25,
									 16, 15, 14, 13, 12, 11, 10,  9,  8,
									  0,  1,  2,  3,  4,  5,  6,  7};

const char * WORDS[] = {"nasty woman", "biggly", "barb", "higgs", "crossvalidation", "p value", "grant", "brainz", "mit", "amygdala", "gablab", "mri", "python", "nipype"};


void min_max_delay(uint16_t min, uint16_t max)
{
	uint16_t delay = min + randomize_int(max - min);
	PIOS_DELAY_WaitmS(delay);
}

void flicker_led(uint8_t led_idx, uint8_t duration)
{
	uint32_t end_time = PIOS_Thread_Systime() + duration;

	while (PIOS_Thread_Systime() < end_time) {
		memset(LEDBUFF, 0, NUMLEDBYTES);
		if (randomize_int(256) < 128) {
			LEDBUFF[3 * led_idx] = COLORS[colors[led_idx]][1];
			LEDBUFF[3 * led_idx + 1] = COLORS[colors[led_idx]][0];
			LEDBUFF[3 * led_idx + 2] = COLORS[colors[led_idx]][2];
		}
		PIOS_RE1FPGA_SetLEDs(LEDBUFF, NUMLEDS);
		min_max_delay(duration / 20, duration / 3);
	}
}

void random_flicker(uint16_t duration)
{
	uint32_t end_time = PIOS_Thread_Systime() + duration;
	uint8_t led_idx;

	while (PIOS_Thread_Systime() < end_time) {
		if (randomize_int(255) < 30) {
			memset(LEDBUFF, 0, NUMLEDBYTES);
		}
		led_idx = randomize_int(NUMLEDS -1);
		flicker_led(led_idx,  randomize_int(100));
	}
}


static void strangerThingsTask(void *parameters)
{
	const char * word;
	uint8_t led_idx;

	for (int i=0; i<NUMLEDS; i++) {
		colors[i] = randomize_int(NUMCOLORS - 1);
	}

	while(1){
		word = WORDS[randomize_int(NELEMENTS(WORDS))];

		for (int i=0; i<strlen(word); i++){
			if (word[i] != ' ') {
				led_idx = LED_ORDER[word[i] - 'a'];
				if (led_idx >= NUMLEDS) {
					// this shouldn't happen
					continue;
				}
				//flicker_led(led_idx, LETTER_DURATIOM_MIN + randomize_int(LETTER_DURATIOM_MAX - LETTER_DURATIOM_MIN));
				memset(LEDBUFF, 0, NUMLEDBYTES);
				LEDBUFF[3 * led_idx] = COLORS[colors[led_idx]][1];
				LEDBUFF[3 * led_idx + 1] = COLORS[colors[led_idx]][0];
				LEDBUFF[3 * led_idx + 2] = COLORS[colors[led_idx]][2];
				PIOS_RE1FPGA_SetLEDs(LEDBUFF, NUMLEDS);
				min_max_delay(LETTER_DURATIOM_MIN, LETTER_DURATIOM_MAX);
			}
			else {
				memset(LEDBUFF, 0, NUMLEDBYTES);
				PIOS_RE1FPGA_SetLEDs(LEDBUFF, NUMLEDS);
				min_max_delay(LETTER_DURATIOM_MIN, LETTER_DURATIOM_MAX);
			}
		}
		//memset(LEDBUFF, 0, NUMLEDBYTES);
		//PIOS_RE1FPGA_SetLEDs(LEDBUFF, NUMLEDS);
		//min_max_delay(LETTER_DURATIOM_MIN, LETTER_DURATIOM_MAX);
		random_flicker(WORD_PAUSE_MIN + randomize_int(WORD_PAUSE_MAX - WORD_PAUSE_MIN));
		memset(LEDBUFF, 0, NUMLEDBYTES);
		PIOS_RE1FPGA_SetLEDs(LEDBUFF, NUMLEDS);
		min_max_delay(WORD_PAUSE_MIN, WORD_PAUSE_MAX);
	};
}

/**
 * @}
 * @}
 */
