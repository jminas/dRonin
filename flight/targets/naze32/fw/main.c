/**
 ******************************************************************************
 * @addtogroup TauLabsTargets Tau Labs Targets
 * @{
 * @addtogroup Naze32 family support files
 * @{
 *
 * @file       main.c 
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @brief      Start FreeRTOS and the Modules.
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
#include "FreeRTOS.h"
#include "task.h"

/* Task Priorities */
#define PRIORITY_TASK_HOOKS             (tskIDLE_PRIORITY + 3)

/* Global Variables */

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);
extern void Stack_Change(void);

/* Local Variables */
#define INIT_TASK_PRIORITY	(tskIDLE_PRIORITY + configMAX_PRIORITIES - 1)	// max priority
#define INIT_TASK_STACK		(640 / 4)
static xTaskHandle initTaskHandle;

/* Function Prototypes */
static void initTask(void *parameters);

/* Prototype of generated InitModules() function */
extern void InitModules(void);

/* board-info/system_stm32f10x.c */
extern void SetSysClock(void);

/**
* dRonin Main function:
*
* Initialize PiOS<BR>
* Create the "System" task (SystemModInitializein Modules/System/systemmod.c) <BR>
* Start FreeRTOS Scheduler (vTaskStartScheduler)<BR>
* If something goes wrong, blink LED1 and LED2 every 100ms
*
*/
int main()
{
	int	result;

	/* NOTE: Do NOT modify the following start-up sequence */
	/* Any new initialization functions should be added in OpenPilotInit() */
	vPortInitialiseBlocks();

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();

	// Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers
	// Configure the Flash Latency cycles and enable prefetch buffer
	SetSysClock();

	/* use a FreeRTOS task to bring up the system so we can */
	/* always rely on FreeRTOS primitive */
	result = xTaskCreate(initTask, (const signed char *)"init",
						 INIT_TASK_STACK, NULL, INIT_TASK_PRIORITY,
						 &initTaskHandle);
	PIOS_Assert(result == pdPASS);

	/* swap the stack to use the IRQ stack */
	Stack_Change();

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be running. */
	/* Do some PIOS_LED_HEARTBEAT to user that something bad just happened */
	PIOS_ANNUNC_Off(PIOS_LED_HEARTBEAT); \
	for(;;) { \
		PIOS_ANNUNC_Toggle(PIOS_LED_HEARTBEAT); \
		PIOS_DELAY_WaitmS(100); \
	};

	return 0;
}
/**
 * Initialisation task.
 *
 * Runs board and module initialisation, then terminates.
 */
void
initTask(void *parameters)
{
	/* board driver init */
	PIOS_Board_Init();

	/* Initialize modules */
	MODULE_INITIALISE_ALL(PIOS_WDG_Clear);

	/* terminate this task */
	vTaskDelete(NULL);
}

/**
 * @}
 * @}
 */

