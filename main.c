#include "main.h"
#include "usbcdc.h"
#include "lpc.h"

volatile uint32_t system_millis;

/* Called when systick fires */
void sys_tick_handler(void)
{
	system_millis++;
}

/* Set up a timer to create 1ms ticks. */
static void systick_setup(void)
{
	/* clock rate / 1000 to get 1ms interrupt rate */
	systick_set_reload(rcc_ahb_frequency/1000);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_counter_enable();
	/* this done last */
	systick_interrupt_enable();
}

/* sleep for delay milliseconds */
void msleep(uint32_t delay)
{
	uint32_t wake = system_millis + delay;
	while (wake > system_millis);
}

int main(void) {
  int i;

  rcc_periph_clock_enable(BOARD_RCC_LED);
  LED_ENABLE();
  LED_BUSY();

  rcc_clock_setup_in_hse_8mhz_out_72mhz();
  rcc_periph_clock_enable(RCC_GPIOA); /* For USB */

  rcc_periph_clock_enable(BOARD_RCC_USB_PULLUP);
  rcc_periph_clock_enable(RCC_AFIO); /* For SPI */

#if BOARD_USE_DEBUG_PINS_AS_GPIO
  gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF, AFIO_MAPR_TIM2_REMAP_FULL_REMAP);
#endif

  systick_setup();

/* This for reenumarate USB */
  gpio_set_mode(BOARD_PORT_USB_PULLUP, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BOARD_PIN_USB_PULLUP);
#if BOARD_USB_HIGH_IS_PULLUP
  gpio_set(BOARD_PORT_USB_PULLUP, BOARD_PIN_USB_PULLUP);
#else
  gpio_clear(BOARD_PORT_USB_PULLUP, BOARD_PIN_USB_PULLUP);
#endif /* BOARD_USB_HIGH_IS_PULLUP */

  // wait for usb reenumarate
  msleep(10);

#ifdef DEBUG_SWO_ITM
  trace_setup();
#endif
  printf("start\r");

  usbcdc_init();
  lpc_init();

  /* The loop. */
  while (true) {
    /* Wait and blink if USB is not ready. */
    LED_IDLE();
    while (!usb_ready) {
      printf("wait usb...\r");
      LED_BUSY();
      msleep(1000);
      LED_IDLE();
      msleep(200);
    }

    if(rbuf_len)
    {
      i = handle_serprog((char*)rbuf, rbuf_len);
      if(i == -1)
      {
    	  usb_int_relay();
    	  continue;
      }

	  if (i > 0)
	  {
		  memmove((char*)rbuf, (char*)&rbuf[i], rbuf_len - i);
		  rbuf_len -= i;
	  }
    }
  }

  return 0;
}

/* Interrupts (currently none here) */

static void signal_fault(void) {
  uint32_t i;

  while (true) {
    LED_ENABLE();
    LED_BUSY();
    for (i = 0; i < 5000000; i ++) {
      asm("nop");
    }
    LED_DISABLE();
    for (i = 0; i < 5000000; i ++) {
      asm("nop");
    }
  }
}

void nmi_handler(void)
__attribute__ ((alias ("signal_fault")));

void hard_fault_handler(void)
__attribute__ ((alias ("signal_fault")));
