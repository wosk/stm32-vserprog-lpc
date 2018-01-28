/**
 ******************************************************************************
 * @file      semihosting.c
 * @author    Coocox
 * @version   V1.0
 * @date      09/10/2011
 * @brief     Semihosting LowLayer GetChar/SendChar Implement.
 *
 *******************************************************************************
 */

// always call Hard_fault interrupt and block SysTick
// the most slowest output.. 1000 records by 18 symbols - 35 sec

#include <sys/types.h>
#include <sys/stat.h>
#include "semihosting.h"

#ifdef DEBUG_ENABLE_SEMIHOST

static char g_buf[16];
static unsigned char g_buf_len = 0;

/**************************************************************************//**
  * @brief  prototype: int SH_DoCommand(int n32In_R0, int n32In_R1, int *pn32Out_R0)
  *
  * @param  n32In_R0	R0
  * @param  n32In_R1    R1
  * @param  pn32Out_R0  R2
  *
  * @retval
  * 0: No ICE debug
  * 1: ICE debug
  *
  *****************************************************************************/

//int SH_DoCommand(int n32In_R0, int n32In_R1, int *pn32Out_R0) {
//
//	  __asm volatile (
//			/* Wait ICE or HardFault */
//			/* ICE will step over BKPT directly */
//			/* HardFault will step BKPT and the next line */
//			"  BKPT 0xAB\n"
//	        "  B SH_ICE\n"
//
//			/* Captured by HardFault */
//			/* Set return value to 0 */
//	        "SH_HardFault:\n"
//	        "  MOVS   R0, #0\n"
//			"  BX LR\n"
//
//			/* Captured by ICE */
//			/* Save the return value to *pn32Out_R0 */
//	        "SH_ICE:\n"
//	        "    CMP R2, #0\n"
//	        "    BEQ SH_End\n"
//	        "    STR R0, [R2]\n"
//
//			/* Set return value to 1 */
//			"SH_End:\n");
//
//	  return 1;
//}
//
///**************************************************************************//**
//  * @brief  HardFault Handler
//  *
//  * @param  None.
//  *
//  * Skip the semihost command in free run mode.
//  *
//  * @retval None
//  *****************************************************************************/
//void HardFault_Handler(void){
//
//	__asm volatile(
//			"LDR    R0, [R13, #24]\n"         	/* Get previous PC */
//			"LDRH   R1, [R0]\n"               	/* Get instruction */
//			"LDR    R2, =0xBEAB\n"            	/* The sepcial BKPT instruction */
//			"CMP    R1, R2\n"                 	/* Test if the instruction at previous PC is BKPT 0xAB */
//			"BNE    HardFault_Handler_Ret\n"  	/* Not BKPT */
//
//			"ADDS   R0, #4\n"                 	/* Skip BKPT and next line */
//			"STR    R0, [R13, #24]\n"         	/* Save previous PC */
//
//			"BX     LR\n"
//			);
//}

/**************************************************************************//**
 * @brief  Transmit a char on semihosting mode.
 *
 * @param  ch is the char that to send.
 *
 * @return Character to write.
 *****************************************************************************/
void SH_SendChar(char ch) {
	g_buf[g_buf_len++] = ch;
	g_buf[g_buf_len] = '\0';
	if (g_buf_len + 1 >= sizeof(g_buf) || ch == '\n' || ch == '\0') {
		g_buf_len = 0;
		/* Send the str */
		if (SH_DoCommand(SYS_WRITE0, (int) g_buf, NULL) != 0) {
			return;
		}
	}
}

/**************************************************************************//**
 * @brief  Transmit a null-terminated string on semihosting mode.
 *
 * @param  str is the string that to send.
 *
 * @return Character to write.
 *****************************************************************************/
void SH_SendString(const char *str)
{
	//int j;
	if (SH_DoCommand(SYS_WRITE0, (int)str, NULL) != 0) {
		return;
	}
}

/**************************************************************************//**
 * @brief  Read a char on semihosting mode.
 *
 * @param  None.
 *
 * @return Character that have read.
 *****************************************************************************/
char SH_GetChar() {
	int nRet;

	while (SH_DoCommand(0x101, 0, &nRet) != 0) {
		if (nRet != 0) {
			SH_DoCommand(SYS_READC, 0, &nRet);
			return (char) nRet;
		}
	}

	return 0;
}
#endif //DEBUG_ENABLE_SEMIHOST
