/*
 * traceswo.c
 *
 *  Created on: 3 янв. 2018 г.
 *      Author: wosk
 */

#include <libopencm3/stm32/dbgmcu.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/tpiu.h>
#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/rcc.h>

#include "traceswo.h"

#define stimulus_port 1

// Max freq is 6Mhz
#define SWO_FREQ 1000000

void trace_setup()
{
	// * Configure the TPIU and assign TRACE I/Os
	/* Enable trace subsystem (we'll use ITM and TPIU). */
	SCS_DEMCR |= SCS_DEMCR_TRCENA;


	/* Use Manchester code for asynchronous transmission. */
	// its default
	//TPIU_SPPR = TPIU_SPPR_ASYNC_MANCHESTER;
	//TPIU_SPPR = TPIU_SPPR_ASYNC_NRZ;
	//tmp = (rcc_apb2_frequency/SWO_FREQ) - 1;
	//TPIU_ACPR = tmp;

	/* Formatter and flush control. */
	// disable formatter
	//TPIU_FFCR &= ~TPIU_FFCR_ENFCONT;

	/* Enable TRACESWO pin for async mode. */
	// Asynchronous Trace through (PB3 /JTDO/ TRACESWO)
	DBGMCU_CR = DBGMCU_CR_TRACE_IOEN | DBGMCU_CR_TRACE_MODE_ASYNC;

	ITM_LAR = 0xC5ACCE55;

	//ITM_TCR = (1 << 16) | ITM_TCR_ITMENA;

	// Write 0x00010005 to the ITM Trace Control Register to enable the ITM with Sync
	// enabled and an ATB ID different from 0x00
	//ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_TraceBusID_Msk;
	ITM_TCR = ITM_TCR_ITMENA | ITM_TCR_SYNCENA | ITM_TCR_TRACE_BUS_ID_MASK;

	// Write 0x1 to the ITM Trace Enable Register to enable the Stimulus Port
	ITM_TER[0] = 1 << stimulus_port;

	// Write 0x1 to the ITM Trace Privilege Register to unmask stimulus ports 7:0
	ITM_TPR = 1 << (stimulus_port/8);
}

void trace_sendstr(const char *s)
{
	while (*s){
		while (!(ITM_STIM8(stimulus_port) & ITM_STIM_FIFOREADY));
		ITM_STIM8(stimulus_port) = *s++;
	}
}

void trace_char(char c)
{
	while (!(ITM_STIM8(stimulus_port) & ITM_STIM_FIFOREADY));
	ITM_STIM8(stimulus_port) = c;

}
