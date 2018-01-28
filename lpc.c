/*
 * lpc.c
 *
 *  Created on: 2 янв. 2018 г.
 *      Author: wosk
 */

#include "main.h"

#define GPIO_OUTPUT_MODE GPIO_MODE_OUTPUT_50_MHZ

void lpc_init() {

	rcc_periph_clock_enable(BOARD_RCC_LPC_LAD_PINS);
	rcc_periph_clock_enable(BOARD_RCC_LPC_LFRAME);
	rcc_periph_clock_enable(BOARD_RCC_LPC_LCLK);
	rcc_periph_clock_enable(BOARD_RCC_LPC_RST);
	// LAD[0:3]
	gpio_set_mode(BOARD_PORT_LPC_LAD, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LAD0);
	gpio_set_mode(BOARD_PORT_LPC_LAD, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LAD1);
	gpio_set_mode(BOARD_PORT_LPC_LAD, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LAD2);
	gpio_set_mode(BOARD_PORT_LPC_LAD, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LAD3);
	// LFRAME#
	gpio_set_mode(BOARD_PORT_LPC_LFRAME, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LFRAME);
	// RST#
	gpio_set_mode(BOARD_PORT_LPC_RST, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_RST);
	// LCLK#
	gpio_set_mode(BOARD_PORT_LPC_LCLK, GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LCLK);

	gpio_clear(BOARD_PORT_LPC_RST, BOARD_PIN_RST);
	msleep(1);
	gpio_set(BOARD_PORT_LPC_RST, BOARD_PIN_RST);
}

////////nibble interface ///////////////////////

#define clock_low() gpio_clear(BOARD_PORT_LPC_LCLK, BOARD_PIN_LCLK)
#define clock_high() gpio_set(BOARD_PORT_LPC_LCLK, BOARD_PIN_LCLK)

#define lframe_low() gpio_clear(BOARD_PORT_LPC_LFRAME, BOARD_PIN_LFRAME)
#define lframe_high() gpio_set(BOARD_PORT_LPC_LFRAME, BOARD_PIN_LFRAME)

#define delay() asm volatile("nop")


void clock_cycle(void) {
	clock_low();
	delay();
	clock_high();
}

void nibble_set_dir(uint8_t dir, uint8_t mode) {
	gpio_set_mode(BOARD_PORT_LPC_LAD, dir, mode,
			BOARD_PIN_LAD0 | BOARD_PIN_LAD1 | BOARD_PIN_LAD2 | BOARD_PIN_LAD3);
#ifdef DEBUG
	if(mode == GPIO_CNF_INPUT_PULL_UPDOWN){
		gpio_set(BOARD_PORT_LPC_LAD,
				BOARD_PIN_LAD0 | BOARD_PIN_LAD1);
		gpio_clear(BOARD_PORT_LPC_LAD,
					BOARD_PIN_LAD2 | BOARD_PIN_LAD3);
	}
#endif
}

#define nibble_output() nibble_set_dir(GPIO_OUTPUT_MODE, GPIO_CNF_OUTPUT_PUSHPULL)
#define nibble_input() nibble_set_dir(GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN)

void nibble_write(uint8_t data) {
#if((BOARD_PIN_LAD0==GPIO0) && (BOARD_PIN_LAD1==GPIO1) && \
	(BOARD_PIN_LAD2==GPIO2) && (BOARD_PIN_LAD3==GPIO3))
	gpio_set(BOARD_PORT_LPC_LAD, (data & 0xF));
#else
	if(data & (1<<0))
		gpio_set(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD0);
	else
		gpio_clear(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD0);

	if(data & (1<<1))
		gpio_set(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD1);
	else
		gpio_clear(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD1);

	if(data & (1<<2))
		gpio_set(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD2);
	else
		gpio_clear(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD2);

	if(data & (1<<3))
		gpio_set(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD3);
	else
		gpio_clear(BOARD_PORT_LPC_LAD, BOARD_PIN_LAD3);
#endif
}

uint8_t nibble_read(void) {
	uint8_t data = 0;
	uint32_t r;

	r = gpio_get(BOARD_PORT_LPC_LAD,
			BOARD_PIN_LAD0 | BOARD_PIN_LAD1 | BOARD_PIN_LAD2 | BOARD_PIN_LAD3);

#if((BOARD_PIN_LAD0==GPIO0) && (BOARD_PIN_LAD1==GPIO1) && \
	(BOARD_PIN_LAD2==GPIO2) && (BOARD_PIN_LAD3==GPIO3))
	data = r & 0xF;
#else
	data |= ((r & BOARD_PIN_LAD0) ? (1<<0) : 0);
	data |= ((r & BOARD_PIN_LAD1) ? (1<<1) : 0);
	data |= ((r & BOARD_PIN_LAD2) ? (1<<2) : 0);
	data |= ((r & BOARD_PIN_LAD3) ? (1<<3) : 0);
#endif
	return data;
}

void nibble_start(uint8_t byte) {
	lframe_high();
	clock_low();
	lframe_low();
	nibble_write(byte);
	clock_cycle();
	lframe_high();
}

void clocked_nibble_write(uint8_t value) {
	clock_low();
	nibble_write(value);
	clock_high();
}

uint8_t clocked_nibble_read(void) {
	uint8_t r = nibble_read();
	clock_cycle();
	return r;
}

////////LPC interface ///////////////////////
#define LPC_START 0b0000
#define LPC_CYCTYPE_READ 0b0100
#define LPC_CYCTYPE_WRITE 0b0110
#define LPC_TAR 0b1111
#define LPC_SYNC 0b0000
#define LPC_SYNC_SHORT_WAIT 0b0101

#define lpc_nibble_write(v) clocked_nibble_write(v)
#define lpc_nibble_read() clocked_nibble_read()

static void lpc_start(void) {
	nibble_start(LPC_START);
}

static void lpc_send_addr(uint32_t addr) {
	addr |= 0xFF000000;
	for (int i=4; i <= sizeof(addr)*8; i+=4){
		lpc_nibble_write((uint8_t)(addr >> (32-i)));
	}
}

int lpc_write_address(uint32_t addr, uint8_t byte) {
	uint8_t r;
	uint8_t i=10;

	lpc_start(); 					// 1 START
	lpc_nibble_write(LPC_CYCTYPE_WRITE); // 2 TYPE
	lpc_send_addr(addr); 			// 3-10 ADDR
	lpc_nibble_write(byte & 0xF); // 11 Data[3:0]
	lpc_nibble_write(byte >> 4);  // 12 Data[7:4]
	lpc_nibble_write(LPC_TAR);	  // 13 TAR0
	nibble_input();
	clock_cycle();				  // 14 TAR1

	do{
		r = lpc_nibble_read(); // 15 SYNC
//		if(r == LPC_SYNC_SHORT_WAIT)
//			continue;
		if (!i--)
			return -1;
	} while(LPC_SYNC != r);

	clock_cycle();				 // 16 TAR0
	clock_cycle();				 // 17 TAR1
	nibble_output();
	return 0;
}

int lpc_read_address(uint32_t addr, uint8_t *byte) {
	uint8_t r;
	uint8_t i=10;
	uint8_t data;

	lpc_start();					// 1 START
	lpc_nibble_write(LPC_CYCTYPE_READ); // 2 TYPE
	lpc_send_addr(addr);			// 3-10 ADDR
	lpc_nibble_write(LPC_TAR);	  	// 11 TAR0
	nibble_input();
	clock_cycle();					// 12 TAR1

	// 13 SYNC
	do{
		r = lpc_nibble_read();
//		if(r == LPC_SYNC_SHORT_WAIT)
//			continue;
		if (!i--) return -1;
	} while(LPC_SYNC != r);

	data = lpc_nibble_read(); 	// 14 Data[3:0]
	data |= lpc_nibble_read()<<4;  // 15 Data[7:4]
	clock_cycle();					// 16 TAR0
	clock_cycle();					// 17 TAR1
	nibble_output();

	if(byte != 0)
		*byte = data;
	return 0;
}

int lpc_test(void) {
	lpc_init();
	if (lpc_read_address(0xFFFFFFFF, 0) == -1) return 0;
	return 1;
}
