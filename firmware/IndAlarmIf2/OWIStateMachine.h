#ifndef OWISTATEMACHINE_H
#define OWISTATEMACHINE_H

unsigned char OWI_StateMachine();
extern signed int temperature;

// Defines used only in code example.
#define OWI_STATE_IDLE                  'i'
#define OWI_STATE_DETECT_PRESENCE1      'p'
#define OWI_STATE_WAIT_FOR_CONVERSION1  'w'
#define OWI_STATE_WAIT_FOR_CONVERSION2  'W'
#define OWI_STATE_DETECT_PRESENCE2      'P'
#define OWI_STATE_READ_SCRATCHPAD       's'
#define OWI_STATE_CHECK_CRC             'c'
#define OWI_STATE_CONVERTED_T           't'


#endif // OWISTATEMACHINE_H
