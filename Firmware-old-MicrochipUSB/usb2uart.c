/*
*
*	USB infrared remote control receiver transmitter firmware v1.0
*	License: creative commons - attribution, share-alike 
*	Copyright Ian Lesnet 2010
*	http://dangerousprototypes.com
*
*/

#include "globals.h"


typedef enum
{
	SM_USB_UART_CONFIG_MODE=0,
	SM_USB_UART_CONFIG_MODE_OK,
	SM_USB_UART_RUN_MODE,
	SM_USB_UART_FLUSH_BUFFER

}_Sm_Usb_Uart;

static _Sm_Usb_Uart  Sm_Usb_Uart;
static u8 Configbuffctr;


static u8 TxBuffer[BUFSZ],TxBufferCtrIn,TxBufferCtrOut;
static volatile u8 RxBuffer[BUFSZ],RxBufferCtrIn,RxBufferCtrOut;


//setup for USB UART
void Usb2UartSetup(void){
if(mUSBUSARTIsTxTrfReady())
	{
	irToy.usbOut[0]='U';//answer OK
	irToy.usbOut[1]='0';
	irToy.usbOut[2]='1';
	putUnsignedCharArrayUsbUsart(irToy.usbOut,3);
	}
Sm_Usb_Uart=SM_USB_UART_CONFIG_MODE;
Configbuffctr=0;
}



#if defined(USB_CDC_SET_LINE_CODING_HANDLER)
void mySetLineCodingHandler(void)
{
//If the request is not in a valid range
if(cdc_notice.GetLineCoding.dwDTERate.Val > 115200)
	{
	USBStallEndpoint(0,1);
	}
else
	{
	DWORD_VAL dwBaud;

	CDCSetBaudRate(cdc_notice.GetLineCoding.dwDTERate.Val);

	dwBaud.Val = (CLOCK_FREQ/4)/line_coding.dwDTERate.Val-1;

	SPBRG = dwBaud.v[0];
	SPBRGH = dwBaud.v[1];
	}
}
#endif


void Usb2Uart_InitUart(u8 InitRx)
{
TRISC|=		0xC0;
TXSTA=		0x24;
RCSTA=(InitRx==TRUE)? 0x90:0x00;
//RCSTA=		0x90;
BAUDCON=	0x08;
PIE1=		0x20;
ResetUsbUartTxBuffers();
ResetUsbUartRxBuffers();

}



void Usb2Uart_CloseUart(void)
{
TXSTA=0;
RCSTA=0;
}

#if 0
u8 Usb2UartPrepareTxData(void)
{
if(getUnsignedCharArrayUsbUart(UsbRxDataBuffer,1))
	{
	TxBuffer[TxBufferCtrIn]=UsbRxDataBuffer[0];
	TxBufferCtrIn++;
	TxBufferCtrIn&=USBUARTBUFCTRMASK;
	return TRUE;
	}
return FALSE;
}


void Usb2UartSendTxDataFromBuff(void)
{
if((TxIf)&&(TxBufferCtrIn!=TxBufferCtrOut))  // If Uart is not full and no data to be sent
	{
	TXREG=TxBuffer[TxBufferCtrOut];
	TxBufferCtrOut++;
	TxBufferCtrOut&=USBUARTBUFCTRMASK;
	}
}
#endif


// to go back to the other default, user must unplug USB IR Toy
u8 Usb2UartService(void)
{
static u8 buff_config[5];

switch (Sm_Usb_Uart)
	{
	// get configuration data
	case SM_USB_UART_CONFIG_MODE:
		{
		FlushUsbRx();
		Usb2Uart_InitUart(TRUE);
//		TRISC|=		0xC0;
//		TXSTA=		0x24;
//		RCSTA=		0x90;
//		BAUDCON=	0x08;
//		PIE1=		0x20;
		//ResetUsbUartTxBuffers();
		//ResetUsbUartRxBuffers();
		Sm_Usb_Uart=SM_USB_UART_CONFIG_MODE_OK;
		break;
		}


	case SM_USB_UART_CONFIG_MODE_OK:
		{
		Sm_Usb_Uart=SM_USB_UART_RUN_MODE;
		break;
		}

	case SM_USB_UART_RUN_MODE:
		{
#if 1
		if(getUnsignedCharArrayUsbUart(UsbRxDataBuffer,1))
			{
			TxBuffer[TxBufferCtrIn]=UsbRxDataBuffer[0];
			TxBufferCtrIn++;
			TxBufferCtrIn&=USBUARTBUFCTRMASK;
			}
#endif

		if(RxBufferCtrIn!=RxBufferCtrOut)
		//if ((Usb2UartPrepareTxData()==FALSE) && (RxBufferCtrIn!=RxBufferCtrOut))
			{
			if( mUSBUSARTIsTxTrfReady() )
				{
				LAT_LED_PIN^=1; // toggle led every sending
				irToy.usbOut[0]=RxBuffer[RxBufferCtrOut];//answer OK
				putUnsignedCharArrayUsbUsart(irToy.usbOut,1);

				RxBufferCtrOut++;
				RxBufferCtrOut&=USBUARTBUFCTRMASK;
				}
			}
		break;
		}
	} // end of switch


//	Usb2UartSendTxDataFromBuff();
#if 1
	if((TxIf)&&(TxBufferCtrIn!=TxBufferCtrOut))  // If Uart is not full and no data to be sent
		{
		TXREG=TxBuffer[TxBufferCtrOut];
		TxBufferCtrOut++;
		TxBufferCtrOut&=USBUARTBUFCTRMASK;
		}
#endif
	if(RCSTA&0x06) // error handling
		{
		RCSTAbits.CREN=0;
		RCSTAbits.CREN=1;
		}

// this will contain the routine for receiving and transmit
return 0;//CONTINUE
}





//high priority interrupt routine
#pragma interrupt Usb2UartInterruptHandlerHigh
void Usb2UartInterruptHandlerHigh (void)
{
// this will contain the RX interrupt FIFO Buffering
if(RcIf)
	{
	RxBuffer[RxBufferCtrIn]=RCREG;
	RxBufferCtrIn++;
	RxBufferCtrIn&=USBUARTBUFCTRMASK;
	}
}


