/*
 * lpc.h
 *
 *  Created on: 2 янв. 2018 г.
 *      Author: wosk
 */

#ifndef LPC_H_
#define LPC_H_

void lpc_init();
int lpc_write_address(uint32_t addr, uint8_t byte);
int lpc_read_address(uint32_t addr, uint8_t *byte);
int lpc_test(void);

#endif /* LPC_H_ */
