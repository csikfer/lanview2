// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               OWIIntFunctions.h
* \li Compiler:           IAR EWAAVR 3.20a
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All AVRs with UART or USART module. 
*
* \li Application Note:   AVR318 - Dallas 1-Wire(R) master.
*                         
*
* \li Description:        Header file for OWIIntFunctions.c 
*
*                         $Revision: 1.7 $
*                         $Date: Thursday, August 19, 2004 14:27:18 UTC $
*                           Modify by Csiki Ferenc 2012.
****************************************************************************/

#ifndef _OWI_INTE_BYTE_FUNCTIONS_H_
#define _OWI_INTE_BYTE_FUNCTIONS_H_


/****************************************************************************
 Prototypes
****************************************************************************/
void OWI_Init(void);
void OWI_DetectPresence( void );
void OWI_TransmitData(unsigned char * data, unsigned char dataLength);
void OWI_ReceiveData(unsigned char * data, unsigned char dataLength);


/*! \brief  Union that holds 1-Wire flags.
 *  
 *  This union holds the 3 1-Wire flags. In addition it declares a 
 *  variable called allFlags that can be used to clear all flags.
 */
typedef union 
{
    unsigned char allFlags; //!< Variable used to clear all flags.
    struct {
        unsigned char busy:1; //!< 1 bit wide bitfield used to signal that the 1-Wire driver i busy.
        unsigned char presenceDetected:1; //!< 1 bit wide bitfield used to reflect whether presence was detected.
        unsigned char error:1; //!< 1 bit wide bitfield used to flag error.
    }   bits;
} OWIflags;



#endif
