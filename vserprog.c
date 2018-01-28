#include "main.h"
#include "serprog.h"
#include "usbcdc.h"
#include "lpc.h"

#define S_IFACE_VERSION   0x01             /* Currently version 1 */
#define S_PGM_NAME        "stm32-vserprog" /* The program's name, must < 16 bytes */
#define S_SUPPORTED_BUS   (BUS_LPC)
#define S_CMD_MAP ( \
  (1 << S_CMD_NOP)       	| \
  (1 << S_CMD_Q_IFACE)   	| \
  (1 << S_CMD_Q_CMDMAP)  	| \
  (1 << S_CMD_Q_PGMNAME) 	| \
  (1 << S_CMD_Q_SERBUF) 	| \
  (1 << S_CMD_Q_BUSTYPE) 	| \
  (1 << S_CMD_Q_WRNMAXLEN) 	| \
  (1 << S_CMD_Q_RDNMAXLEN) 	| \
  (1 << S_CMD_Q_OPBUF) 		| \
  (1 << S_CMD_SYNCNOP)   	| \
  (1 << S_CMD_O_DELAY)   	| \
  (1 << S_CMD_O_INIT)    	| \
  (1 << S_CMD_O_EXEC)    	| \
  (1 << S_CMD_R_BYTE)   	| \
  (1 << S_CMD_R_NBYTES)   	| \
  (1 << S_CMD_O_WRITEB)   	| \
  (1 << S_CMD_S_BUSTYPE) 	  \
)

#define S_OPBUFLEN 200
static unsigned char opbuf[S_OPBUFLEN];
static unsigned int opbuf_bytes = 0;

static unsigned char opbuf_addbyte(unsigned char c) {
	if (opbuf_bytes == S_OPBUFLEN) return 1;
	opbuf[opbuf_bytes++] = c;
	return 0;
}

static int exec_opbuf()
{
	uint32_t ptr; /* opbuf ptr*/
	uint32_t adr, len;
	uint8_t op, val, resp;

	printf("exec_opbuf:\r");

	for(ptr=0; ptr<opbuf_bytes; )
	{
		op = opbuf[ptr++];
		switch (op)
		{
			case S_CMD_O_DELAY:
				len = opbuf[ptr++];
				len |= (opbuf[ptr++]<<8);
				len |= (opbuf[ptr++]<<16);
				len |= (opbuf[ptr++]<<24);
				printf("sleep %d msec\r", len);
			    msleep((uint32_t)((len+999)/1000));
			    resp = S_ACK;
				break;

			case S_CMD_O_WRITEB:
				adr = opbuf[ptr++];
				adr |= (opbuf[ptr++]<<8);
				adr |= (opbuf[ptr++]<<16);
				val = opbuf[ptr++];
				printf("wr 0x%06x: 0x%02x\r", adr, val);
				if (0 != lpc_write_address(adr, val)){
					resp = S_NAK;
				}
				else
					resp = S_ACK;
				break;

			case S_CMD_O_WRITEN:
				len = opbuf[ptr++];
				len |= (opbuf[ptr++]<<8);
				len |= (opbuf[ptr++]<<16);

				adr = opbuf[ptr++];
				adr |= (opbuf[ptr++]<<8);
				adr |= (opbuf[ptr++]<<16);

				resp = S_ACK;

				printf("wrn adr 0x%06x len %d\r", adr, len);
				while(len--)
				{
					if(0 != lpc_write_address(adr++, opbuf[ptr++]))
					{
						ptr += len;
						resp = S_NAK;
						break;
					}
				}
				break;

			default:
				printf("unkn op cmd %d\r", op);
				continue;
		}
		  if(resp == S_ACK)
			  printf("resp S_ACK\r");
		  else
			  printf("resp s_NAK\r");
		usbcdc_write(&resp, 1);
	}

	printf("exec_opbuf end.\r");
	return 0;
}

int handle_serprog(volatile char req[], size_t reqlen) {
  uint32_t   i;        /* Loop                */
  uint32_t   l;        /* Length              */
  uint32_t  slen;     /* SPIOP write length  */
  uint32_t  rlen;     /* SPIOP read length   */
  uint32_t  freq_req; /* Requested SPI clock */
  uint8_t command = req[0];
  uint8_t resp[USBCDC_PKT_SIZE_DAT];
  uint8_t r = 0;	/* ptr for resp buffer */
  uint8_t req_h = 1; // request bytes handled
  uint8_t busype_allowed = BUS_NONE;

  LED_BUSY();

  switch(command) {
    case S_CMD_NOP: {
      printf("S_CMD_NOP\r");
      resp[r++] = S_ACK;
      break;
    }

    case S_CMD_Q_IFACE: {
      printf("S_CMD_Q_IFACE\r");
      resp[r++] = S_ACK;

      /* little endian multibyte value to complete to 16bit */
      resp[r++] = S_IFACE_VERSION;
      resp[r++] = 0;
      break;
    }

    case S_CMD_Q_CMDMAP: {
      printf("S_CMD_Q_CMDMAP\r");
      resp[r++] = S_ACK;

      resp[r++] = S_CMD_MAP & 0xFF;
      resp[r++] = S_CMD_MAP >> 8 & 0xFF;
      resp[r++] = S_CMD_MAP >> 16 & 0xFF;
      resp[r++] = S_CMD_MAP >> 24 & 0xFF;

      for(i = 0; i < 32 - sizeof(uint32_t); i++) {
    	  resp[r++] = 0;
      }
      break;
    }

    case S_CMD_Q_PGMNAME: {
      printf("S_CMD_Q_PGMNAME\r");
      resp[r++] = S_ACK;

      l = 0;
      while(S_PGM_NAME[l]) {
    	resp[r++] = S_PGM_NAME[l];
        l ++;
      }

      for(i = l; i < 16; i++) {
    	  resp[r++] = 0;
      }
      break;
    }

    case S_CMD_Q_SERBUF: {
      printf("S_CMD_Q_SERBUF\r");
      resp[r++] = S_ACK;

      /* Pretend to be 64K (0xffff) */
      resp[r++] = sizeof(rbuf) & 0xFF;
      resp[r++] = sizeof(rbuf) >>8;
      break;
    }

    case S_CMD_Q_BUSTYPE: {
      printf("S_CMD_Q_BUSTYPE\r");
      resp[r++] = S_ACK;
      resp[r++] = S_SUPPORTED_BUS;
      break;
    }

    case S_CMD_Q_CHIPSIZE: {
    	printf("S_CMD_Q_CHIPSIZE\r");
      break;
    }

    case S_CMD_Q_OPBUF: {
      printf("S_CMD_Q_OPBUF\r");
      resp[r++] = S_ACK;
      //operation buffer size
      resp[r++] = sizeof(opbuf) & 0xff;
      resp[r++] = sizeof(opbuf) >> 8;
      break;
    }

    case S_CMD_Q_WRNMAXLEN: {
      printf("S_CMD_Q_WRNMAXLEN\r");
      resp[r++] = S_ACK;
      resp[r++] = sizeof(rbuf) & 0xFF;
      resp[r++] = sizeof(rbuf) >>8;
      resp[r++] = 0;
      break;
    }

    case S_CMD_Q_RDNMAXLEN: {
      printf("S_CMD_Q_RDNMAXLEN\r");
      resp[r++] = S_ACK;
      resp[r++] = 60;//USBCDC_PKT_SIZE_DAT - 1/*S_ACK*/;
      resp[r++] = 0;
      resp[r++] = 0;
      break;
    }

    case S_CMD_R_BYTE: {
    	uint8_t byte;
      if(reqlen < 4 )
          	  return -1;

      req_h = 4;
      //addr
      i = req[1] | (req[2]<<8) | (req[3]<<16);

      printf("S_CMD_R_BYTE 0x%08x\r", i);

      if(0 != lpc_read_address(i, &byte)){
    	  resp[r++] = S_NAK;
      }
      else
      {
    	  resp[r++] = S_ACK;
    	  resp[r++] = byte;
      }
      break;
    }

    case S_CMD_R_NBYTES: {
      printf("S_CMD_R_NBYTES\r");
      if(reqlen < 7 )
          	  return -1;

      req_h = 7;
      //addr
      i = req[1] | (req[2]<<8) | (req[3]<<16);
      //len
      l = req[4] | (req[5]<<8) | (req[6]<<16);
      printf("addr 0x%08x len %d\r",i, l);

      resp[r++] = S_ACK;
      while(l--){
    	  if((r >= sizeof(resp)) || lpc_read_address(i++, &resp[r++])){
    		  resp[0] = S_NAK;
    		  r=1;
    		  break;
    	  }
      }
      break;
    }

    case S_CMD_O_INIT: {
      printf("S_CMD_O_INIT\r");
      opbuf_bytes = 0;
      resp[r++] = S_ACK;
      break;
    }

    case S_CMD_O_WRITEB: {
    	printf("S_CMD_O_WRITEB\r");

        if(reqlen < 5 )
        	return -1;

        req_h = 5;

        if ( opbuf_addbyte(S_CMD_O_WRITEB) ||
      	   opbuf_addbyte(req[1]) ||
  		   opbuf_addbyte(req[2]) ||
  		   opbuf_addbyte(req[3]) ||
  		   opbuf_addbyte(req[4]))
        {}

      break;
    }

    case S_CMD_O_WRITEN: {
    	printf("S_CMD_O_WRITEN\r");
    	//len
    	l = req[1] | (req[2]<<8) | (req[3]<<16);

    	if(reqlen < 7+l )
    	    return -1;

    	req_h = 7+l;

        if ( opbuf_addbyte(S_CMD_O_WRITEN) ||
      	   opbuf_addbyte(req[1]) ||
  		   opbuf_addbyte(req[2]) ||
  		   opbuf_addbyte(req[3]) ||
  		   opbuf_addbyte(req[4]) ||
		   opbuf_addbyte(req[5]) ||
		   opbuf_addbyte(req[6]))
        {
        	break;
        }

        for(i=0; i<l;i++)
        {
        	if( opbuf_addbyte(req[7+i]) )
        		break;
        }
      break;
    }

    case S_CMD_O_DELAY: {
      i = req[1] | (req[2]<<8) | (req[3]<<16) | (req[4]<<24); //usecs
      printf("S_CMD_O_DELAY %u usecs\r", i);

  	  if(reqlen < 5 )
  	    return -1;

  	  req_h = 5;

      if ( opbuf_addbyte(S_CMD_O_DELAY) ||
    	   opbuf_addbyte(req[1]) ||
		   opbuf_addbyte(req[2]) ||
		   opbuf_addbyte(req[3]) ||
		   opbuf_addbyte(req[4]))
      { }

      break;
    }

    case S_CMD_O_EXEC: {
      printf("S_CMD_O_EXEC\r");
      if(exec_opbuf())
    	  resp[r++] = S_NAK;
      else
    	  resp[r++] = S_ACK;

  	  opbuf_bytes = 0;
      break;
    }

    case S_CMD_SYNCNOP: {
      printf("S_CMD_SYNCNOP\r");
      resp[r++] = S_NAK;
      resp[r++] = S_ACK;
      break;
    }

    case S_CMD_S_BUSTYPE: {
      uint8_t c = req[1];
      if(reqlen < 2 )
    	  return -1;

      printf("S_CMD_S_BUSTYPE %x\r", c);
      req_h = 2;

      if(c & ~S_SUPPORTED_BUS){
    	  // exist unsupported bustype
    	  resp[r++] = S_NAK;
    	  break;
      }

      busype_allowed = c;
      resp[r++] = S_ACK;

      if(busype_allowed & BUS_LPC){
    	  lpc_test();
      }

      break;
    }

    case S_CMD_O_SPIOP: {
      printf("S_CMD_O_SPIOP\r");
      slen = req[1] | (req[2]<<8) | (req[3]<<16);
      rlen = req[4] | (req[5]<<8) | (req[6]<<16);

      if(reqlen < 7+slen )
    	  return -1;
      req_h = 7+slen;
      //TODO
      resp[r++] = S_NAK;
      break;
    }

    case S_CMD_S_SPI_FREQ: {
      printf("S_CMD_S_SPI_FREQ\r");
      if(reqlen < 5 )
          	  return -1;
      req_h = 5;
      freq_req = req[1] | (req[2]<<8) | (req[3]<<16) | (req[4]<<24);

      //TODO
      resp[r++] = S_NAK;

      if(freq_req == 0) {
    	resp[r++] = S_NAK;
      } else {
    	resp[r++] = S_ACK;
    	r += 4;
      }

      break;
    }

    case S_CMD_S_PIN_STATE: {
    	if(reqlen < 2 )
    		return -1;
    	req_h = 2;
      // TODO: OE
      break;
    }

    default: {
      printf("cmd 0x%x discarded\r", command);
      LED_IDLE();
      return req_h;
    }
  }

  if(r) {
	  if(resp[0] == S_ACK)
		  printf("resp S_ACK %d\r", r);
	  else
		  printf("resp s_NAK %d\r", r);

	  usbcdc_write(resp, r);
  }

  LED_IDLE();
  return req_h;
}


