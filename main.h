#ifndef MAIN_H_
#define MAIN_H_

#include <stdbool.h>
#include <stdio.h>
#include <string.h> // memmove

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/systick.h>

#include "board.h"

#define LED_ENABLE()  gpio_set_mode(BOARD_PORT_LED, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_LED)
#define LED_DISABLE() gpio_set_mode(BOARD_PORT_LED, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BOARD_PIN_LED)

#if BOARD_LED_HIGH_IS_BUSY
#define LED_BUSY()    gpio_set(BOARD_PORT_LED, BOARD_PIN_LED)
#define LED_IDLE()    gpio_clear(BOARD_PORT_LED, BOARD_PIN_LED)
#else
#define LED_BUSY()    gpio_clear(BOARD_PORT_LED, BOARD_PIN_LED)
#define LED_IDLE()    gpio_set(BOARD_PORT_LED, BOARD_PIN_LED)
#endif /* BOARD_LED_HIGH_IS_BUSY */

#ifndef DEBUG
#undef printf
#define printf
#endif

int handle_serprog(volatile char req[], size_t reqlen);
void msleep(uint32_t delay);

#endif /* MAIN_H_ */
