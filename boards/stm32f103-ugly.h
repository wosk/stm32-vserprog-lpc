#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdbool.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/*
 * Board definitions for dev-kit Stm32f103c8t6 ugly boards
 * http://wiki.stm32duino.com/index.php?title=Ugly_board
 */

#define BOARD_USE_DEBUG_PINS_AS_GPIO false

#define BOARD_RCC_LED                RCC_GPIOA
#define BOARD_PORT_LED               GPIOA
#define BOARD_PIN_LED                GPIO0
#define BOARD_LED_HIGH_IS_BUSY       false

#define BOARD_RCC_USB_PULLUP         RCC_GPIOA
#define BOARD_PORT_USB_PULLUP        GPIOA
#define BOARD_PIN_USB_PULLUP         GPIO12
#define BOARD_USB_HIGH_IS_PULLUP     false

/*
 * LPC pins
*/
#define BOARD_RCC_LPC_LAD_PINS	RCC_GPIOA
#define BOARD_PORT_LPC_LAD	GPIOA
#define BOARD_PIN_LAD0		GPIO1
#define BOARD_PIN_LAD1		GPIO3
#define BOARD_PIN_LAD2		GPIO5
#define BOARD_PIN_LAD3		GPIO7

#define BOARD_RCC_LPC_LFRAME	RCC_GPIOB
#define BOARD_PORT_LPC_LFRAME	GPIOB
#define BOARD_PIN_LFRAME	GPIO15

#define BOARD_RCC_LPC_LCLK	RCC_GPIOA
#define BOARD_PORT_LPC_LCLK	GPIOA
#define BOARD_PIN_LCLK		GPIO9

#define BOARD_RCC_LPC_RST	RCC_GPIOB
#define BOARD_PORT_LPC_RST	GPIOB
#define BOARD_PIN_RST		GPIO13

#endif /* __BOARD_H__ */
