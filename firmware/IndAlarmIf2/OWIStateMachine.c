// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               main.c
* \li Compiler:           IAR EWAAVR 3.20a
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All AVRs with UART or USART module.
*
* \li Application Note:   AVR318 - Dallas 1-Wire(R) master.
*
*
* \li Description:        Example on how to use the interrupt-driven
*                         UART implementation of the 1-Wire(R) protocol.
*
*                         $Revision: 1.7 $
*                         $Date: Thursday, August 19, 2004 14:27:16 UTC $
* @li Mod by csikfer      2013.01.06.
*                         compiler AVRGCC
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "OWIIntFunctions.h"
#include "OWIInterruptDriven.h"
#include "OWIcrc.h"
#include "OWIStateMachine.h"

#include "indalarmif2.h"

#define FALSE       0
#define TRUE        1

#define DS1820_START_CONVERSION         0x44    //!< DS1820 start conversion command
#define DS1820_READ_SCRATCHPAD          0xbe    //!< DS1820 Read scratchpad command

extern OWIflags OWIStatus;
extern unsigned char *OWIDataBuffer;

signed int temperature = -256;


/*! \brief  Code example for the interrupt-driven driver.
 *
 *  This code example assumes that there is a DS1820 temperature sensor
 *  connected to the 1-Wire bus. The temperature sensor is continously
 *  polled, and the temperature reading is output to PORTB.
 */
/*
void main(void)
{

    OWI_Init();

    __enable_interrupt();

    // Configure PORTB as output. This can be used to output debugging values
    // to the LEDs on the STK500 development board.
    DDRB = 0xff;

    for(;;)
    {
        // If the 1-Wire(R) bus is not busy, run the state machine.
        if (!OWIStatus.bits.busy)
        {
            OWI_StateMachine();
        }

        // Do something else while 1-Wire(R) bus is busy.
        PORTB = ~(temperature >> 1);
    }
}
*/

/*! \brief  The state machine that controls communication on the 1-Wire bus
 *
 *  This function is called from main every time the 1-Wire driver is not
 *  busy. The state machine will read the temperature from a DS1820 temperature
 *  sensor, crc check it, and put it in the global variable temperature if
 *  everything is OK.
 */
unsigned char OWI_StateMachine()
{
    static unsigned char state = OWI_STATE_IDLE;
    static unsigned char buf[9];
    unsigned char i;
    unsigned char crc;

    // If an error has occurred since last time, clear all flags and
    // return to idle state.
    if (OWIStatus.bits.error)
    {
        state = OWI_STATE_IDLE;
        OWIStatus.allFlags = FALSE;
        PRNC('E');
    }

    switch (state)
    {
        case OWI_STATE_IDLE:
        {
            // Send reset signal and update state.
            OWI_DetectPresence();
            state = OWI_STATE_DETECT_PRESENCE1;
            break;
        }

        case OWI_STATE_DETECT_PRESENCE1:
        {
            // If no presence was detected, go back to idle state.
            if(OWIStatus.bits.presenceDetected == FALSE)
            {
                state = OWI_STATE_IDLE;
            }
            // If presence was detected, send Skip ROM and Start conversion
            // signals.
            else
            {
                buf[0] = OWI_ROM_SKIP;
                buf[1] = DS1820_START_CONVERSION;
                OWI_TransmitData(buf, 16);
                state = OWI_STATE_WAIT_FOR_CONVERSION1;
            }
            break;
        }

        case OWI_STATE_WAIT_FOR_CONVERSION1:
        {
            // Receive one byte of data to check for completion of the
            // temperature conversion.
            OWI_ReceiveData(buf, 8);
            state = OWI_STATE_WAIT_FOR_CONVERSION2;
            break;
        }

        case OWI_STATE_WAIT_FOR_CONVERSION2:
        {
            // If everything received was zero. Jump to the last state
            // to receive a new byte.
            if (buf[0] == 0x00)
            {
                state = OWI_STATE_WAIT_FOR_CONVERSION1;
            }
            // If there was at least 1 one received, continue with a new
            // reset.
            else
            {
                OWI_DetectPresence();
                state = OWI_STATE_DETECT_PRESENCE2;
            }
            break;
        }

        case OWI_STATE_DETECT_PRESENCE2:
        {
            // If no presence was detected, go back to idle state.
            if(OWIStatus.bits.presenceDetected == FALSE)
            {
                state = OWI_STATE_IDLE;
            }
            // If presence was detected, send Skip ROM and Read scratchpad
            // signals.
            else
            {
                buf[0] = OWI_ROM_SKIP;
                buf[1] = DS1820_READ_SCRATCHPAD;
                OWI_TransmitData(buf, 16);
                state = OWI_STATE_READ_SCRATCHPAD;
            }
            break;
        }

        case OWI_STATE_READ_SCRATCHPAD:
        {
            // Read the 9 bytes of scratchpad data.
            OWI_ReceiveData(buf, 9 * 8);
            state = OWI_STATE_CHECK_CRC;
            break;
        }

        case OWI_STATE_CHECK_CRC:
        {
            // Compare the computed crc with the crc read from the
            // scratchpad.
            crc = 0;
            for(i = 0; i < 8; i++)
            {
                crc =  OWI_ComputeCRC8(buf[i], crc);
            }
            // If they match, update the temperature variable.
            if (crc == buf[8])
            {
                temperature = buf[0] | (buf[1] << 8);
                state = OWI_STATE_IDLE;
//              PRNP2("\nT = %d.%d\n", temperature / 2, (temperature & 1) ?  5 : 0);
                return OWI_STATE_CONVERTED_T;
            }
            // If they don't match, go back to the second Reset to
            // read the scratchpad again.
            else
            {
                OWI_DetectPresence();
                state = OWI_STATE_DETECT_PRESENCE2;
            }
            break;
        }
            default:
            PRNP1("Invalid State : %c\n", state);
    }
    return state;
}
