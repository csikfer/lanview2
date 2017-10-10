// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               OWIInterruptDriven.h
* \li Compiler:           IAR EWAAVR 3.20a
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All AVRs with UART or USART module. 
*
* \li Application Note:   AVR318 - Dallas 1-Wire(R) master.
*                         
*
* \li Description:        Defines used in the interrupt-driven 
*                         1-Wire(R) driver.
*                         
*                         $Revision: 1.7 $
*                         $Date: Thursday, August 19, 2004 14:27:16 UTC $
*                           Modify by Csiki Ferenc 2012.
****************************************************************************/

#ifndef _OWI_INTERRUPT_H_
#define _OWI_INTERRUPT_H_

#include "OWIdefs.h"
#include "OWIDeviceSpecific.h"


/***************************************************************
 User defines
***************************************************************/
/*! Use U(S)ART double speed
 *
 *  Set this define to '1' to enable U(S)ART double speed. More 
 *  information can be found in the data sheet of the AVR.
 *
 *  \note   The UART Baud Rate Register settings are also affected 
 *          by this setting.
 */
#define     OWI_UART_2X         1

/*! UART Baud Rate register setting that results in 115200 Baud
 *
 *  This define should be set to the UBRR value that will generate
 *  a Baud rate of 115200. See data sheet for more information and 
 *  examples of Baud rate settings.
 */
#define     OWI_UBRR_115200     7


/*! UART Baud Rate register setting that results in 9600 Baud
 *
 *  This define should be set to the UBRR value that will generate
 *  a Baud rate of 9600. See data sheet for more information and 
 *  examples of Baud rate settings.
 */
#define     OWI_UBRR_9600       95



/***************************************************************
 Other defines
***************************************************************/
#define     TRUE    1
#define     FALSE   0

#endif
