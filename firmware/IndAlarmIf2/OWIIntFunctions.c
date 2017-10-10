// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               OWIIntFunctions.c
* \li Compiler:           IAR EWAAVR 3.20a
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All AVRs with UART or USART module. 
*
* \li Application Note:   AVR318 - Dallas 1-Wire(R) master.
*                         
*
* \li Description:        Functions and Interrupt Service Routines that 
*                         handles automated transmission and reception of 0-255 
*                         bits. 
*
*                         $Revision: 1.7 $
*                         $Date: Thursday, August 19, 2004 14:27:18 UTC $
****************************************************************************/

#include "OWIIntFunctions.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include "OWIInterruptDriven.h"

OWIflags OWIStatus;
unsigned char *OWIDataBuffer;
unsigned char OWIBufferLength;


/*! \brief  Initialization of the U(S)ART for 1-Wire communication.
 *  
 *  This function configures the U(S)ART for 1-Wire communication.
 *
 *  \note   Parameters are set in "OWIInterruptDriven.h"
 */
void OWI_Init(void)
{
    // Choose single or double UART speed.
    OWI_UART_STATCTRL_REG_A = (OWI_UART_2X << OWI_U2X);
    
    // Enable UART receiver, transmitter and interrupt on receive;
    OWI_UART_STATCTRL_REG_B = (1 << OWI_RXEN) | (1 << OWI_TXEN) | (1 << OWI_RXCIE);

    // Set up asynchronous mode, 8 data bits, no parity, 1 stop bit.
    // (Initial value, can be removed)
    #ifdef URSEL
    OWI_UART_STATCTRL_REG_C = (1 << OWI_URSEL) | (1 << OWI_UCSZ1) | (1 << OWI_UCSZ0);
    #else
    OWI_UART_STATCTRL_REG_C = (1 << OWI_UCSZ1) | (1 << OWI_UCSZ0);
    #endif
    
    // Clear OWI flags.
    OWIStatus.allFlags = FALSE;
}


/*! \brief   Send the Reset signal on 1-Wire bus to receive presence
 *  signal from slave devices.
 * 
 *
 *  This function configures the UART module for transmission of the
 *  Reset signal. The busy flag is TRUE until the 1-Wire driver has 
 *  completed the Reset/Presence sequence. when the busy flag becomes 
 *  FALSE, the presence flag indicates whether a Presence signal was 
 *  received.
 */
void OWI_DetectPresence(void)
{
    // Set OWI busy flag
    OWIStatus.bits.busy = TRUE;
    
    // Set the BAUD Rate to 9600 bps for reset/presence signalling.
    OWI_UART_BAUD_RATE_REG_L = OWI_UBRR_9600;
    
    // Enable UART data register empty interrupt to start the UART
    // transmitter.
    OWI_UART_STATCTRL_REG_B |= (1 << OWI_UDRIE);
}


/*! \brief  Transmit data on the 1-Wire bus. 
 *
 *  This function configures the UART module for transmission of data.
 *  The busy flag is TRUE until transmission is complete.
 *
 *  \param  data        Pointer to the data to be transmitted.
 *  
 *  \param  dataLength  The number of bits to be transmitted.
 */
void OWI_TransmitData(unsigned char * data, unsigned char dataLength)
{
    // Set OWI busy flag
    OWIStatus.bits.busy = TRUE;
    
    // Set the data buffer pointer and data length.
    OWIDataBuffer = data;
    OWIBufferLength = dataLength;
    
    // Enable UART data register empty interrupt to start the UART
    // transmitter.    
    OWI_UART_STATCTRL_REG_B |= (1 << OWI_UDRIE);
}


/*! \brief  Receive data from the 1-Wire bus.
 *
 *  This function configures the UART module for reception of data.
 *  The busy flag is TRUE until reception is complete.
 *
 *  \param  data        A pointer to the location in memory where the 
 *                      received data should be placed
 *
 *  \param  dataLength  The number of bits to receive.
 */
void OWI_ReceiveData(unsigned char * data, unsigned char dataLength)
{
    unsigned char numBytes;

    // Fill buffer with 1's (Write 1 and Read waveforms are similar).
    numBytes = dataLength >> 3;
    if (dataLength & 0x07)
    {
        numBytes++;
    }
    
    while(numBytes)
    {
        data[numBytes - 1] = 0xff;
        numBytes--;
    }

    // Transmit the '1'-filled buffer on the bus to receive data.
    OWI_TransmitData(data, dataLength);
}


/*! \brief  UART Receive Complete Interrupt Service Routine
 *
 *  This ISR takes care of reception and interpretation of signals
 *  from the 1-Wire bus.
 */
ISR(OWI_UART_RXC_VECT) // #pragma vector = OWI_UART_RXC_VECT __interrupt void UART_RXC_ISR()
{
    static unsigned char bitsReceived = 0;
    static unsigned char receiveBuffer = 0;
    static unsigned char byteIndex = 0;
    unsigned char waveform;
    
    // Check for frame error.
    if (OWI_UART_STATCTRL_REG_A & (1 << OWI_FE))
    {
        // Frame error.
        // Could be caused by a slave connecting to the bus, noice
        // or a short on the bus pulling it low.

        // Read UART DATA REGISTER to clear FE and RXC flags.
        waveform = OWI_UART_DATA_REGISTER;
        
        // Flag error, and clean up.
        OWIStatus.bits.error = TRUE;
        OWIStatus.bits.busy = FALSE;
        OWI_UART_STATCTRL_REG_B &= ~(1 << OWI_UDRIE);
        return;
    }    

    // Read the UART data register to get the waveform for this bit.
    waveform = OWI_UART_DATA_REGISTER;
    
    // If the Baud rate is 9600, check for presence signal.
    if (OWI_UART_BAUD_RATE_REG_L == OWI_UBRR_9600)
    {
        // Set presenceDetected flag if waveform is different from 
        // what was transmitted on the bus.
        OWIStatus.bits.presenceDetected = (waveform != OWI_UART_RESET);

        // Set UART baud rate to 115200 bps.
        OWI_UART_BAUD_RATE_REG_L = OWI_UBRR_115200;

        // Reset bits received counter.
        bitsReceived = 0;

        // Set busy flag to FALSE.
        OWIStatus.bits.busy = FALSE;
    }
    else // Baud rate != 9600.
    {
        // Increase bits received counter.
        bitsReceived++;

        // Right shift receive buffer.
        receiveBuffer >>= 1;

        // Check value of received bit.
        if (waveform == OWI_UART_READ_BIT) // Received 1.
        {
            // Set msb of receive buffer
            receiveBuffer |= 0x80;
        }
        else // Received 0.
        {
            // Do nothing, a 0 has already been shifted in.
        }

        // Was this the last bit?
        if (bitsReceived == OWIBufferLength)
        {
            // Right shift receive buffer to align bits.
            // Only needed when the number of bits is not divisible by 8.
            while(bitsReceived++ & 0x07)  
            {
                receiveBuffer >>= 1;   
            }
            // Insert the received data in the OWI Data buffer.
            OWIDataBuffer[byteIndex] = receiveBuffer;
            
            // Clean up.
            bitsReceived = 0;
            byteIndex = 0;
            OWIStatus.bits.busy = FALSE;
        }
        else // Not the last bit.
        {
            // Is bitsReceived divisible with 8?
            // If so, one byte has been received and the receive buffer
            // must be placed in the big OWI data buffer. 
            if (!(bitsReceived & 0x07))
            {
                // Put the received byte in the data buffer and increment byte index.
                OWIDataBuffer[byteIndex] = receiveBuffer;                
                byteIndex++;
            }
        }
    }    
    return;
}


/*! \brief  UART Data Register Empty Interrupt Service Routine
 *
 *  This ISR is run every time the UART transmit buffer is empty, thus
 *  automating the process of transmitting and receiving bits.
 */
ISR(OWI_UART_UDRE_VECT) // #pragma vector = OWI_UART_UDRE_VECT __interrupt void UART_UDRE_ISR()
{
    static unsigned char bitsSent = 0;
    static unsigned char transmitBuffer;
    static unsigned char byteIndex = 0;
    
    // If the UART Baud Rate is 9600 bps, send the Reset signal.
    if (OWI_UART_BAUD_RATE_REG_L == OWI_UBRR_9600)
    {
        //Send Reset signal.
        OWI_UART_DATA_REGISTER = OWI_UART_RESET;
        
        // Reset bits sent counter.
        bitsSent = 0;

        // Clear UDRIE flag.
        OWI_UART_STATCTRL_REG_B &= ~(1 << OWI_UDRIE);

        return;
    }
    
    // If this is the first bit in a new transmission, fetch
    // the first byte from the OWI data buffer and reset
    // byte index.
    if (bitsSent == 0)
    {
        byteIndex = 0;
        transmitBuffer = OWIDataBuffer[0];        
    } 
    
    // If this is the last bit in the transmission, clean up.  
    if (bitsSent == OWIBufferLength)
    {
        bitsSent = 0;
        OWI_UART_STATCTRL_REG_B &= ~(1 << OWI_UDRIE);
    }
    else // Not the last bit.
    {
        // Check lsb of the transmit buffer and transmit corresponding
        // waveform on the bus.
        if (transmitBuffer & 0x01)
        {
            OWI_UART_DATA_REGISTER = OWI_UART_WRITE1;
        }
        else
        {
            OWI_UART_DATA_REGISTER = OWI_UART_WRITE0;
        }
        
        // Right shift transmit buffer.
        transmitBuffer >>= 1;
        
        // Increase bits sent counter.
        bitsSent++;

        // If the number of bits sent is divisible by 8, a whole byte has
        // been shifted out. Fetch a new from the OWI data buffer.
        if (!(bitsSent & 0x07))
        {
            byteIndex++;
            transmitBuffer = OWIDataBuffer[byteIndex];
        }
    }
    return;
}
