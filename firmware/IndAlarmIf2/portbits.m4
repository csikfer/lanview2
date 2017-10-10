define(pbdef, `
#define $1_DDR  DDR$2
#define $1_PIN  PIN$2
#define $1_POU  PORT$2
#define $1_BIT  $3
')

define(pdef, `
#define $1_DDR  DDR$2
#define $1_PIN  PIN$2
#define $1_POU  PORT$2
')

// LED port bit definíciók
pbdef(GRN1, B, 5) dnl kétszínű LED alsó zöld
pbdef(RED1, B, 6) dnl kétszínű LED alsó piros
pbdef(GRN2, B, 7) dnl kétszínű LED felső zöld
pbdef(RED2, H, 3) dnl kétszínű LED felső piros
pbdef(BLUE, H, 4) dnl egy színű LED (hiányozhat) Blue

// LED A négyszög kimenet a kétszínű LED-ekhez közös pont
pbdef(LA, D, 5)
pdef(LBL, G)
pdef(LBH, E)
#define	LBL_MSK	0x0f
#define LBH_MSK 0xf0

// RS485 IO engedályező portok
pbdef(OE, D, 6)
pbdef(IE, D, 7)

// Jumperek, GPIO, ...
pbdef(J0, D, 4)
pbdef(J1, G, 4)
pbdef(J2, G, 5)
pbdef(J3, H, 2)
pbdef(J4, H, 7)
pbdef(J5, B, 0)
pbdef(J6, H, 5)
pbdef(SCL, D, 0)
pbdef(SPA, B, 4)
pbdef(SPB, H, 6)
pbdef(SDA, D, 1)

// jumper, kapcsolók
pbdef(S1, E, 2)
pbdef(S2, E, 3)

// Az SPI mint port
pbdef(PDI, B, 2)
pbdef(PDO, B, 3)
pbdef(SCK, B, 1)

// Érzékelők kimeneti portok
pdef(SAL, A)
pdef(SAH, C)
pdef(SBL, J)
pdef(SBH, L)
