// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               OWIcrc.h
* \li Compiler:           IAR EWAAVR 3.20a
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All AVRs.
*
* \li Application Note:   AVR318 - Dallas 1-Wire(R) master.
*                         
*
* \li Description:        Header file for OWIUtils.c
*
*                         $Revision: 1.7 $
*                         $Date: Thursday, August 19, 2004 14:27:16 UTC $
****************************************************************************/

#ifndef _OWI_UTILS_H_
#define _OWI_UTILS_H_


#define     OWI_CRC_OK      0x00    //!< CRC check succeded
#define     OWI_CRC_ERROR   0x01    //!< CRC check failed


unsigned char OWI_ComputeCRC8(unsigned char inData, unsigned char seed);
unsigned int OWI_ComputeCRC16(unsigned char inData, unsigned int seed);
unsigned char OWI_CheckRomCRC(unsigned char * romValue);


#endif
