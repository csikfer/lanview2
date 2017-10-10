



// LED port bit definíciók

#define GRN1_DDR  DDRB
#define GRN1_PIN  PINB
#define GRN1_POU  PORTB
#define GRN1_BIT  5
 
#define RED1_DDR  DDRB
#define RED1_PIN  PINB
#define RED1_POU  PORTB
#define RED1_BIT  6
 
#define GRN2_DDR  DDRB
#define GRN2_PIN  PINB
#define GRN2_POU  PORTB
#define GRN2_BIT  7
 
#define RED2_DDR  DDRH
#define RED2_PIN  PINH
#define RED2_POU  PORTH
#define RED2_BIT  3
 
#define BLUE_DDR  DDRH
#define BLUE_PIN  PINH
#define BLUE_POU  PORTH
#define BLUE_BIT  4
 
// LED A négyszög kimenet a kétszínű LED-ekhez közös pont

#define LA_DDR  DDRD
#define LA_PIN  PIND
#define LA_POU  PORTD
#define LA_BIT  5


#define LBL_DDR  DDRG
#define LBL_PIN  PING
#define LBL_POU  PORTG


#define LBH_DDR  DDRE
#define LBH_PIN  PINE
#define LBH_POU  PORTE

#define	LBL_MSK	0x0f
#define LBH_MSK 0xf0

// RS485 IO engedályező portok

#define OE_DDR  DDRD
#define OE_PIN  PIND
#define OE_POU  PORTD
#define OE_BIT  6


#define IE_DDR  DDRD
#define IE_PIN  PIND
#define IE_POU  PORTD
#define IE_BIT  7


// Jumperek, GPIO, ...

#define J0_DDR  DDRD
#define J0_PIN  PIND
#define J0_POU  PORTD
#define J0_BIT  4


#define J1_DDR  DDRG
#define J1_PIN  PING
#define J1_POU  PORTG
#define J1_BIT  4


#define J2_DDR  DDRG
#define J2_PIN  PING
#define J2_POU  PORTG
#define J2_BIT  5


#define J3_DDR  DDRH
#define J3_PIN  PINH
#define J3_POU  PORTH
#define J3_BIT  2


#define J4_DDR  DDRH
#define J4_PIN  PINH
#define J4_POU  PORTH
#define J4_BIT  7


#define J5_DDR  DDRB
#define J5_PIN  PINB
#define J5_POU  PORTB
#define J5_BIT  0


#define J6_DDR  DDRH
#define J6_PIN  PINH
#define J6_POU  PORTH
#define J6_BIT  5


#define SCL_DDR  DDRD
#define SCL_PIN  PIND
#define SCL_POU  PORTD
#define SCL_BIT  0


#define SPA_DDR  DDRB
#define SPA_PIN  PINB
#define SPA_POU  PORTB
#define SPA_BIT  4


#define SPB_DDR  DDRH
#define SPB_PIN  PINH
#define SPB_POU  PORTH
#define SPB_BIT  6


#define SDA_DDR  DDRD
#define SDA_PIN  PIND
#define SDA_POU  PORTD
#define SDA_BIT  1


// jumper, kapcsolók

#define S1_DDR  DDRE
#define S1_PIN  PINE
#define S1_POU  PORTE
#define S1_BIT  2


#define S2_DDR  DDRE
#define S2_PIN  PINE
#define S2_POU  PORTE
#define S2_BIT  3


// Az SPI mint port

#define PDI_DDR  DDRB
#define PDI_PIN  PINB
#define PDI_POU  PORTB
#define PDI_BIT  2


#define PDO_DDR  DDRB
#define PDO_PIN  PINB
#define PDO_POU  PORTB
#define PDO_BIT  3


#define SCK_DDR  DDRB
#define SCK_PIN  PINB
#define SCK_POU  PORTB
#define SCK_BIT  1


// Érzékelők kimeneti portok

#define SAL_DDR  DDRA
#define SAL_PIN  PINA
#define SAL_POU  PORTA


#define SAH_DDR  DDRC
#define SAH_PIN  PINC
#define SAH_POU  PORTC


#define SBL_DDR  DDRJ
#define SBL_PIN  PINJ
#define SBL_POU  PORTJ


#define SBH_DDR  DDRL
#define SBH_PIN  PINL
#define SBH_POU  PORTL

