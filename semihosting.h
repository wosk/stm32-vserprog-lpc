/**
 ******************************************************************************
 * @file      semihosting.h
 * @author    Coocox
 * @version   V1.0
 * @date      09/10/2011
 * @brief     Semihosting Low Layer GetChar/SendChar API.
 *
 *******************************************************************************
 */
#ifndef __SIMIHOSTTING_IMPL
#define __SIMIHOSTTING_IMPL

#ifdef DEBUG_ENABLE_SEMIHOST
/********************************************************************************************************//**
 * Semihosting functions prototype
************************************************************************************************************/
//extern int SH_DoCommand(int n32In_R0, int n32In_R1, int *pn32Out_R0);
void SH_SendChar(char ch);
void SH_SendString(const char *str);
char SH_GetChar(void);

typedef enum{

	SYS_OPEN = 0x01,
	SYS_CLOSE,
	SYS_WRITEC, // Write character to console
	SYS_WRITE0, // Write string to console
	SYS_WRITE, // write buffer to the file
	SYS_READ,
	SYS_READC,
	SYS_ISERROR,
	SYS_ISTTY,
	SYS_SEEK, //A

	SYS_FLEN = 0x0C,
	SYS_TMPNAM,
	SYS_REMOVE,

	SYS_CLOCK = 0x10,
	SYS_TIME,
	SYS_SYSTEM,
	SYS_ERRNO

	//...

} SH_Command;


static inline __attribute__ ((naked))
int SH_DoCommand(int n32In_R0, int n32In_R1, int *pn32Out_R0)
{
	__asm volatile (
		/* Wait ICE or HardFault */
		/* ICE will step over BKPT directly */
		/* HardFault will step BKPT and the next line */
		"  BKPT 0xAB\n"
		"  B SH_ICE\n"

		/* Captured by HardFault */
		/* Set return value to 0 */
		"SH_HardFault:\n"
		"  MOVS   R0, #0\n"
		"  BX LR\n"

		/* Captured by ICE */
		/* Save the return value to *pn32Out_R0 */
		"SH_ICE:\n"
		"    CMP R2, #0\n"
		"    BEQ SH_End\n"
		"    STR R0, [R2]\n"

		"SH_End:\n"
		"MOVS R0, #1\n"         /* Set return value to 1 */
		"BX LR\n"           	/* Return */);
}

static inline __attribute__ ((interrupt))
void HardFault_Handler(void)
{
	__asm volatile(
		"LDR    R0, [R13, #24]\n"         	/* Get previous PC */
		"LDRH   R1, [R0]\n"               	/* Get instruction */
		"LDR    R2, =0xBEAB\n"            	/* The sepcial BKPT instruction */
		"CMP    R1, R2\n"                 	/* Test if the instruction at previous PC is BKPT 0xAB */
		"BNE    HardFault_Handler_Ret\n"  	/* Not BKPT */

		"ADDS   R0, #4\n"                 	/* Skip BKPT and next line */
		"STR    R0, [R13, #24]\n"         	/* Save previous PC */

		"BX     LR\n"
		);
}
#endif //DEBUG_ENABLE_SEMIHOST
#endif


