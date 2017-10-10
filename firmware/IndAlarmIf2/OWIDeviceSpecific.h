// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               OWIDeviceSpecific.h
* \li Compiler:           IAR EWAAVR 3.20a
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All AVRs with UART or USART module. 
*
* \li Application Note:   AVR318 - Dallas 1-Wire(R) master.
*                         
*
* \li Description:        Device specific defines that expands to correct
*                         register and bit definition names for the selected
*                         device.
*
*                         $Revision: 1.7 $
*                         $Date: Thursday, August 19, 2004 14:27:16 UTC $

****************************************************************************/

#ifndef _OWI_DEVICE_SPECIFIC_H_
#define _OWI_DEVICE_SPECIFIC_H_

/// AtMega640,AtMega1280,AtMega2560 / USART2

#define OWI_UART_STATCTRL_REG_A     UCSR2A
#define OWI_UART_STATCTRL_REG_B     UCSR2B
#define OWI_UART_STATCTRL_REG_C     UCSR2C
#define OWI_UART_DATA_REGISTER      UDR2
#define OWI_UART_BAUD_RATE_REG_L    UBRR2L
#define OWI_UART_BAUD_RATE_REG_H    UBRR2H

#define OWI_U2X                     U2X2
#define OWI_RXEN                    RXEN2
#define OWI_TXEN                    TXEN2
#define OWI_RXCIE                   RXCIE2
#define OWI_UCSZ1                   UCSZ21
#define OWI_UCSZ0                   UCSZ20
#define OWI_UDRIE                   UDRIE2
#define OWI_FE                      FE2
#define OWI_RXC                     RXC2

#define OWI_UART_RXC_VECT           USART2_RX_vect
#define OWI_UART_UDRE_VECT          USART2_UDRE_vect




#endif
