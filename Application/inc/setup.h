#ifndef __SETUP
#define __SETUP

#include "stm32f4xx_hal.h"

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define u64 uint64_t
#define s64 int64_t


#define bool u8
#define true 1
#define false 0


#define EE1024

#define EEPROM_TYPE_I2C 1
#define EEPROM_TYPE_SPI 2
#define __EEPROM_TYPE__ 2



#define SW_VERSION 224
// 224 2023-04-30 fixed G1 features
// 223 2023-01-16 added G1 features
// 222 2022-12-12 changed pin code from left justified HEX number to decimal (NetQX_wiegand.c)
// 221 2022-11-16 added enable/disable auto unlock with MSG_UNLK 252/253
// 220 2022-11-10 first version with M95M04 4Mb chip
// 212 2021-11-22 short on reader 4 middle pins on power up clears door 1 flags to get out of elevator mode
// 211 2021-11-13 fixed elevator software
// 210 2021-11-11 fixed USART2 interrupt problem
// 209 2021-09-25 fixed flash read/write functionality
// 207 2021-08-18 improved two-man + dual-reader operation. made TIM1 interrupt even faster
// 206 2021-08-13 changed system clock to 100MHz, 2nd reader sampled every 45uS
// 205 2021-08-09 fixed dual-reader mode on all 4 doors
// 204 2021-08-07 increased stored events to 4096
// 203 2021-08-07 dual reader mode working
// 202 2021-08-06 Fixed RTC reset problem 
// 201 2021-08-04 RDR2 working. forced door aux 30 seconds. DIP inverted
// 200 2021-07-31 first version for F411, based on 144 on older boards
// 136 2021-02-09 added post key processing pause per door to prevent multi-key break-in attempts
// 135 2021-02-04 added RDR2,RDR4 D0/D1 signal cleanup in NetQX_intr.c
// 134 - added EVT_RemoteLeaveDoorOpen = event 142 for UNLK 255. ignore extra UNLK commands.
// 133 - added event 140 - enable/disable door unlock
// 132 - changed event numbers according to Daniel's request

//========== STM32 housekeeping functions ===================================
// I/O pin functionality definitions for use with SetPinMode()

// port modes
#define PORTMODE_INPUT         0
#define PORTMODE_OUTPUT        1
#define PORTMODE_ALTERNATE     2
#define PORTMODE_ANALOG        3

// output modes
#define OUTMODE_PP    0x00
#define OUTMODE_OD    0x10

// output speed
#define OUTSPEED_400KHz        0x000
#define OUTSPEED_2MHz          0x100
#define OUTSPEED_10MHz         0x200
#define OUTSPEED_40MHz         0x300

// pullup/pulldown
#define PULL_NONE              0x0000
#define PULL_UP                0x1000
#define PULL_DOWN              0x2000

typedef enum
{ GPIO_FMode_AIN         = ( PORTMODE_ANALOG  ),       // analog input or DAC output
  GPIO_FMode_IN_FLOATING = ( PORTMODE_INPUT  ),       // digital input, no pull-up/down resistor
  GPIO_FMode_IPD         = ( PORTMODE_INPUT | PULL_DOWN  ),       // digital input, pull-up resistor
  GPIO_FMode_IPU         = ( PORTMODE_INPUT | PULL_UP    ),       // digital input, pull-down resistor

  GPIO_FMode_p4_Out_PP   = ( PORTMODE_OUTPUT    | OUTSPEED_400KHz | OUTMODE_PP  ),       // 400KHz digital output, push-pull output
  GPIO_FMode_p4_Out_OD   = ( PORTMODE_OUTPUT    | OUTSPEED_400KHz | OUTMODE_OD  ),       // 400KHz digital output, open drain output
  GPIO_FMode_p4_AF_PP    = ( PORTMODE_ALTERNATE | OUTSPEED_400KHz | OUTMODE_PP  ),       // 400KHz digital output, push-pull alternate function output
  GPIO_FMode_p4_AF_OD    = ( PORTMODE_ALTERNATE | OUTSPEED_400KHz | OUTMODE_OD  ),       // 400KHz digital output, open drain alternate function output

  GPIO_FMode_2_Out_PP    = ( PORTMODE_OUTPUT    | OUTSPEED_2MHz   | OUTMODE_PP  ),       // 2MHz digital output, push-pull output
  GPIO_FMode_2_Out_OD    = ( PORTMODE_OUTPUT    | OUTSPEED_2MHz   | OUTMODE_OD  ),       // 2MHz digital output, open drain output
  GPIO_FMode_2_AF_PP     = ( PORTMODE_ALTERNATE | OUTSPEED_2MHz   | OUTMODE_PP  ),       // 2MHz digital output, push-pull alternate function output
  GPIO_FMode_2_AF_OD     = ( PORTMODE_ALTERNATE | OUTSPEED_2MHz   | OUTMODE_OD  ),       // 2MHz digital output, open drain alternate function output

  GPIO_FMode_10_Out_PP   = ( PORTMODE_OUTPUT    | OUTSPEED_10MHz  | OUTMODE_PP  ),       // 10MHz digital output, push-pull output
  GPIO_FMode_10_Out_OD   = ( PORTMODE_OUTPUT    | OUTSPEED_10MHz  | OUTMODE_OD  ),       // 10MHz digital output, open drain output
  GPIO_FMode_10_AF_PP    = ( PORTMODE_ALTERNATE | OUTSPEED_10MHz  | OUTMODE_PP  ),       // 10MHz digital output, push-pull alternate function output
  GPIO_FMode_10_AF_OD    = ( PORTMODE_ALTERNATE | OUTSPEED_10MHz  | OUTMODE_OD  ),       // 10MHz digital output, open drain alternate function output

  GPIO_FMode_40_Out_PP   = ( PORTMODE_OUTPUT    | OUTSPEED_40MHz  | OUTMODE_PP  ),       // 50MHz digital output, push-pull output
  GPIO_FMode_40_Out_OD   = ( PORTMODE_OUTPUT    | OUTSPEED_40MHz  | OUTMODE_OD  ),       // 50MHz digital output, open drain output
  GPIO_FMode_40_AF_PP    = ( PORTMODE_ALTERNATE | OUTSPEED_40MHz  | OUTMODE_PP  ),       // 50MHz digital output, push-pull alternate function output
  GPIO_FMode_40_AF_OD    = ( PORTMODE_ALTERNATE | OUTSPEED_40MHz  | OUTMODE_OD  )        // 50MHz digital output, open drain alternate function output
} GPIOModeFunc_TypeDef;


#define SetOutputHigh(PORTx, Pin) PORTx->BSRR = (uint32_t)1L << Pin
#define SetOutputLow(PORTx, Pin) PORTx->BSRR = (uint32_t)1L << (Pin+16)

#define SetOutputPin(PORTx, Pin, value) \
  if (value)                            \
    SetOutputHigh(PORTx, Pin);          \
  else                                  \
    SetOutputLow(PORTx, Pin)

#define InputPin(PORTx, Pin) ((PORTx->IDR & (1 << Pin)) != 0)

#define OutputToggle(PORTx, Pin) \
    if (InputPin(PORTx, Pin))     \
    { \
    SetOutputLow(PORTx, Pin); \
    } \
    else                         \
    { \
    SetOutputHigh(PORTx, Pin); \
    }

#define EnableEXTI(Pin) EXTI->IMR |= 1 << Pin
#define DisableEXTI(Pin) EXTI->IMR &= ~(1 << Pin)

#define output_drive(x) SetPinMode(x, GPIO_FMode_10_Out_PP)
#define output_float(x) SetPinMode(x, GPIO_FMode_IN_FLOATING);
#define output_od(x) SetPinMode(x, GPIO_FMode_10_Out_OD)
#define input_pullup(x) SetPinMode(x, GPIO_FMode_IPU);
#define input_pulldown(x) SetPinMode(x, GPIO_FMode_IPD);

#define output_low(x)        SetOutputLow(x)
#define output_high(x)       SetOutputHigh(x)
#define output_toggle(x)     OutputToggle(x)
#define output_pin(x, value) SetOutputPin(x, value)
    
#define input(x)        InputPin(x)

#define enable_ext_interrupt(x) EnableEXTI(x)
#define disable_ext_interrupt(x) DisableEXTI(x)
#define clear_tim_interrupt(htim) __HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE)

#define make32(x3,x2,x1,x0) (((u32)x3 << 24) | ((u32)x2 << 16) | ((u32)x1 << 8) | (u32)x0)
#define make16(x1,x0) (((u32)x1 << 8) | (u32)x0)
#define make8(data, byte) ((data >> (byte * 8)) & 0xFF)

//#define test_timer_1ms(flag)    ((Timer_1mS_Flags & (flag)) != 0)
//#define test_timer_10ms(flag)   ((Timer_10mS_Flags & (flag)) != 0)
//#define test_timer_100ms(flag)  ((Timer_100mS_Flags & (flag)) != 0)
//#define test_timer_1sec(flag)   ((Timer_1sec_Flags & (flag)) != 0)
//
//#define clear_timer_1ms(flag)   Timer_1mS_Flags   &= ~(flag)
//#define Timer_10mS_Flags  &= ~(flag)  Timer_10mS_Flags  &= ~(flag)
//#define Timer_100mS_Flags  &= ~(flag) Timer_100mS_Flags &= ~(flag)
//#define Timer_1sec_Flags  &= ~(flag)  Timer_1sec_Flags  &= ~(flag)

void SetPinMode(GPIO_TypeDef* GPIOx, u16 Pin, GPIOModeFunc_TypeDef PortMode);

void NVIC_Configuration(void);
void SysTickConfig(void);



//====== date/time =======================================-
typedef struct  {
                u8 year;
                u8 month;
                u8 day;
                u8 hour;
                u8 minute;
                u8 second;
                } DATE_TIME;


extern u8 Month_special_days[31]; // buffer for a full month of up to 31 days

//========= access level and time zone record definitions ======
typedef struct {
      u8 range[4][2];        // from/to times in 15 minute intervals [][0]=from time; [][1] is to time
    } DZ_RECORD;

typedef struct {
      u8 DOW[7];            // pointers to day zone records, 0=Mon,6=Sun
      u8 SpecialDays;       // pointer to day zone record of special days
    } WZ_RECORD;

typedef struct {
      u8 weekzone[2];    // 2 x (from time / to time) in 15 minute intervals
      u8 ValidDoor[2][16];
    } AL_RECORD;


#define MAX_AL  255
#define MAX_TZ  255
#define MAX_DZ  255

//----------- events ---------------------------------------
typedef __packed struct {
               u8      event_type;
               u32      timestamp;
               u32      ID;
               u32      Code;
               u8      source;
               } EVENT_RECORD;

#define EVENT_REC_SIZE sizeof(EVENT_RECORD)


#include "event_types.h"

//====== doors & keys ==============================================

#define MAX_DOORS              4
#define ELEVATOR_DOORS        64

#define MAX_KEYS               18000
#define MAX_KEY_RECORDS        ((MAX_KEYS * 10) / 9)
#define MAX_EVENTS             4096


typedef __packed struct {
       u32 Code;
       u32 ID;
       u16  PIN;
       u16  FmDate;
       u16  ToDate;
       u8 Flags;
       u8 al[2];
       } KEY_RECORD;

//============ serial EEPROM addresses ====================
#define ADDR_SETUP         16
#define ADDR_TEST         240
#define ADDR_KEYS         256
#define ADDR_DOORS        (((u32)ADDR_KEYS + MAX_KEY_RECORDS * sizeof(KEY_RECORD) + 256) & 0xFFFFFF00L) // 0x42800
#define ADDR_TZ           ((ADDR_DOORS + MAX_DOORS * sizeof(DOOR_RECORD) + 256) & 0xFFFFFF00L)
#define ADDR_AL           ((ADDR_TZ + MAX_TZ * sizeof(WZ_RECORD) + 256) & 0xFFFFFF00L)
#define ADDR_HOLIDAY      ((ADDR_AL + MAX_AL * sizeof(AL_RECORD) + 256) & 0xFFFFFF00L)
#define ADDR_DZ           (ADDR_HOLIDAY + 48)
#define ADDR_EVTPTR       (((ADDR_DZ + MAX_DZ * sizeof(DZ_RECORD))+ 256) & 0xFFFFFF00L)
#define ADDR_EVENTS       (((u32)ADDR_EVTPTR + 1024) & 0xFFFFFF00L)
//#define ADDR_EVTPTR       (0x78000)
//#define ADDR_EVENTS       (0x78100)
#define ADDR_LAST         (ADDR_EVENTS + MAX_EVENTS * sizeof(EVENT_RECORD) + 256)

extern u32 base_doors_setup;


typedef struct {
       u8 create_seq;
       } SETUP_RECORD;


// Special flags
#define KFLAG_Master                  1         // overrides all restrictions. USE WITH CARE.
#define KFLAG_ExtraUnlock             2         // requires more time to open door
#define KFLAG_AutoOpenStart           4         // allow this user to initiate a lock auto-open interval
#define KFLAG_NotifyAdmin             8         // create a notification on PC every time user presents tag to a reader with notification feature enabled
#define KFLAG_NoEntry                16         // do not allow entry to this user, through ANY door.
#define KFLAG_NoExit                 32         // do not allow exit to this user
#define KFLAG_EntryLimit             64         // If Count = zero, do not allow entry. decrement Count byte for each entry
#define KFLAG_PersonalAL            128         // user has a personal access level attached.

//========= lock setup =========================================

#define  APB_EXIT_FLAG     2
#define  APB_ENTRY_FLAG    1



#define GLOBAL_VARIABLES 16
extern u16 global_variable[GLOBAL_VARIABLES];
#define GVAR_UPDOWN_CTR     0
#define GVAR_TRAFFIC_CTR    1

#define LOCAL_PARAMETERS     16
#define LPRM_TWO_MAN_WINDOW   0
#define LPRM_DELAYED_OPEN     1
#define LPRM_TRAFFIC_THRESH   2
#define LPRM_TRAFFIC_COUNT    3
#define LPRM_XFLAGS           4
#define LPRM_ELEVATOR_FLOORS  7


typedef struct  {
        u16  Unlock_time;
        u8   DoorGroup;
        u8   ReaderTZ;
        u8   AutoTZ;
        u8   DisableTZ;
        u32  Flags;
        u8   Reader_type;
        u8   AjarTime;
        u8   Alternate_mode;
        u8   First_digit;
        u8   Number_of_digits;
        u8   DisableDoor; // MSG_ENUL/MSG_DSUL controlled door unlock
        u16  Local_param[LOCAL_PARAMETERS];
        u16  ErrorCheck;
        } DOOR_RECORD;

#define LFLAG_EXIT                0x00000001    // Exit door (default is entry = 0)
#define LFLAG_INTERLOCK           0x00000002    // Interlock - only one door in DoorGroup May be open at any one time
#define LFLAG_HARD_APB            0x00000004    // Hard APB - a user may enter or exit once until executing the opposite action or until timed out. Door may be Entry or Exit and is member of doors in DoorGroup.
#define LFLAG_SOFT_APB            0x00000008    // Soft APB - a user may only enter once until exiting. May exit multiple times. Door may be Entry or Exit and is member of doors in DoorGroup.
#define LFLAG_APB_TOUT            0x00000010    // APB timeout for all users at midnight
#define LFLAG_TRAFFIC_LT          0x00000020    // Traffic light participation. Inc/dec global counter depending on bit 0. Door may be Entry or Exit and is member of doors in DoorGroup.
#define LFLAG_DPOS_RDREN          0x00000040    // Use DPOS input to enable reader processing
#define LFLAG_RTE_NC              0x00000080    // RTE input normally closed (default = normally open)
#define LFLAG_DPOS_NO             0x00000100    // Door position input normally open (default = normally closed)
#define LFLAG_DAJAR_REL2          0x00000200    // Door ajar operates N-channel output 1
#define LFLAG_PIN_CODE            0x00000400    // PIN code following key on Wiegand readers
#define LFLAG_FIRE_DOOR           0x00000800    // Fire door will open on fire indication from NetETH
#define LFLAG_DIS_UNLOCK          0x00001000    // Disable unlock of this door when MSG_DSUL received, and enable following MSG_ENUL
#define LFLAG_TWO_MAN             0x00002000    // Two-man rule. Two valid tags from same user group have to be presented within x seconds to unlock (time window defined in local parameter #15)
#define LFLAG_1ST_VALID           0x00004000    // First valid tag unlocks AutoTZ
#define LFLAG_DELAYED             0x00010000    // Delayed open mode for safes (dead-band time window defined in local parameter #14)
#define LFLAG_DONT_COUNT          0x00020000    // do not down count user entries
#define LFLAG_BUZ_ANY_MESSAGE     0x00080000    // sound buzzer on any message detected on bus
#define LFLAG_IGNORE_FORCED_DOOR  0x00200000    // do not issue EVT_FORCED_DOOR event message
#define LFLAG_2RDR_SINGLE_DOOR    0x00400000    // apply input from two reader to a single door
#define LFLAG_ELEVATOR_CONTROL    0x00800000    // elevator control

#define LFLAG_DUAL_PRESENCE       0x00008000    // room behind door with entry/exit readers requires at least two persons inside. if not, alarm
#define LFLAG_DUAL_ONCE_PER_DAY   0x00100000    // first time in the morning, for alarm disable. rest of day down to 1 person causes alarm
#define LFLAG_DEAD_MAN_SWITCH     0x00040000    // security room. motion on RTE with no one in the room or no motion for 15 minutes with someone in the room

#define LFLAG_BIDIR_MOT           0x01000000    // originally 0x00008000, Operate a bi-dir DC motor using relay2.
#define LFLAG_SHORT_REJECT_BEEPS  0x02000000    // originally 0x00100000, sound a few short beeps on any card reject
#define LFLAG_UNLOCK_BUZ          0x04000000    // originally 0x00040000, buzz whenever a valid unlock occurs


//============ serial RAM addresses ========================
#define ADDR_EVENT_FL_INDEX     32  // 2 byte address of next FLASH block to be filled with events

//====== I/O definitions ===================================
//u8 analog_samples[64], analog_x = 0;

//======- keys and reader =================================-
#define SEARCH_CODE 1
#define SEARCH_ID   2
#define SEARCH_CARD 3


//======- COM1 variables ===================================
#define COM1_RX_LEN 64
#define COM1_TX_LEN 64


//----------- reader ---------------------------------------
#define RDR_MAGCARD 2
#define RDR_WIEGAND 3
#define RDR_RDR4    4

#define MAX_READER_DATA_LEN 60

extern u32 MCR_NibbleCnt1[MAX_DOORS]; //     rdr_interval            // nibble counter
extern u8 MCR_Nibble1[MAX_DOORS]; //        reader_byteidx               // 5 bit nibble accumulator
extern u8 MCR_OddParity1[MAX_DOORS]; //     ReaderFreq            // odd parity 1 counter
extern u32 MCR_Time1[MAX_DOORS]; //          reader_bitmask             // timeout counter
extern u32 MCR_Ones1[MAX_DOORS]; //          ColumnParity   /* consecutive 1 counter */

extern bool MCR_SrchForB1[MAX_DOORS]; //     ; // = RDRstate1.1 // wait for STX nibble
extern bool MCR_CollectData1[MAX_DOORS]; //  ; // = RDRstate1.2 // collect trck II data bits
extern bool MCR_InitStamp1[MAX_DOORS]; //    ; // = RDRstate1.3 // initialization flag
extern bool MCR_InData1[MAX_DOORS]; //       ; // = RDRstate1.4 // input data polarity following interrupt
extern bool MCR_TrackOne1[MAX_DOORS]; //     ; // = RDRstate1.5 // collect track I data bits

// receive modes
#define COM_IDLE 0 //idle
#define COM_WACK 1 //wait for ack
#define COM_RECD 2 //receive data
#define COM_RACK 3 //receive ack
#define PREAMBLE0 0x5A // 1st byte of preamble
#define PREAMBLE1 0xA5 // 2nd is 0x5A
#define ACKBYTE   0xA5

#define KEY_SEND_TIMES 5


//=========== I/O pins =======================================-

#define RS485_EN    GPIOA,11
#define EEPROM_WP   GPIOD,11
#define SF_WP       GPIOC,8

#define READER_LED1         GPIOC,12
#define READER_LED2         GPIOB,7
#define READER_LED3         GPIOB,6
#define READER_LED4         GPIOA,15

#define MCR_CLOCK1          GPIOE,15
#define MCR_CLOCK2          GPIOD,0
#define MCR_CLOCK3          GPIOE,14
#define MCR_CLOCK4          GPIOE,7
#define MCR_DATA1           GPIOB,10
#define MCR_DATA2           GPIOA,8
#define MCR_DATA3           GPIOB,13
#define MCR_DATA4           GPIOC,9

//#define RDR1_D0_INTR      GPIO_PIN_15
//#define RDR1_D0_INTR      GPIO_PIN_10
//#define RDR2_D0_INTR      GPIO_PIN_0
//#define RDR2_D0_INTR      GPIO_PIN_8
//#define RDR3_D0_INTR      GPIO_PIN_14
//#define RDR3_D0_INTR      GPIO_PIN_13
//#define RDR4_D0_INTR      GPIO_PIN_7
//#define RDR4_D0_INTR      GPIO_PIN_9
//
//#define RDR1_D0           GPIOE,15
//#define RDR1_D1           GPIOB,10
//#define RDR2_D0           GPIOD,0
//#define RDR2_D1           GPIOA,8
//#define RDR3_D0           GPIOE,14
//#define RDR3_D1           GPIOB,13
//#define RDR4_D0           GPIOE,7
//#define RDR4_D1           GPIOC,9

#define RDR1_D0_INTR      GPIO_PIN_15
#define RDR1_D1_INTR      GPIO_PIN_10
#define RDR2_D0_INTR      GPIO_PIN_0
#define RDR2_D1_INTR      GPIO_PIN_8
#define RDR3_D0_INTR      GPIO_PIN_14
#define RDR3_D1_INTR      GPIO_PIN_13
#define RDR4_D0_INTR      GPIO_PIN_7
#define RDR4_D1_INTR      GPIO_PIN_9

#define RDR1_D0           GPIOE,15
#define RDR1_D1           GPIOB,10
#define RDR2_D0           GPIOD,0
#define RDR2_D1           GPIOA,8
#define RDR3_D0           GPIOE,14
#define RDR3_D1           GPIOB,13
#define RDR4_D0           GPIOE,7
#define RDR4_D1           GPIOC,9

#define RDR1_D1b          GPIOB,1
#define RDR2_D1b          GPIOB,0
#define RDR3_D1b          GPIOC,5
#define RDR4_D1b          GPIOB,12


// RDR-4 RX pins
#define RDR4_EXTI_RX1     GPIO_PIN_10
#define RDR4_EXTI_RX2     GPIO_PIN_8
#define RDR4_EXTI_RX3     GPIO_PIN_13
#define RDR4_EXTI_RX4     GPIO_PIN_9

#define RDR4_PIN_RX1      GPIOB,10
#define RDR4_PIN_RX2      GPIOA,8
#define RDR4_PIN_RX3      GPIOB,13
#define RDR4_PIN_RX4      GPIOC,9






#define SCL GPIOD,2
#define SDA GPIOD,1



#define ETH_RST             GPIOD,3

#define DIP3                 GPIOE,3
#define DIP4                 GPIOE,4
#define DIP5                 GPIOE,5
#define DIP6                 GPIOE,6
#define DIP7                 GPIOC,13

//#define DISABLE_KEY_TIME     GPIOC,4

//#define LOCK_RELAY1   GPIOB,5
//#define LOCK_RELAY2   GPIOB,4
//#define LOCK_RELAY3   GPIOD,7
//#define LOCK_RELAY4   GPIOD,6
//#define AUX_RELAY1    GPIOD,5
//#define AUX_RELAY2    GPIOB,8
//#define AUX_RELAY3    GPIOB,9
//#define AUX_RELAY4    GPIOE,0
#define   LOCK_RELAY1  GPIOB,5
#define   LOCK_RELAY2  GPIOD,7
#define   LOCK_RELAY3  GPIOD,5
#define   LOCK_RELAY4  GPIOB,9
#define   AUX_RELAY1   GPIOB,4
#define   AUX_RELAY2   GPIOD,6
#define   AUX_RELAY3   GPIOB,8
#define   AUX_RELAY4   GPIOE,0

#define RDR_BUZZER1   GPIOD,8
#define RDR_BUZZER2   GPIOD,13
#define RDR_BUZZER3   GPIOD,14
#define RDR_BUZZER4   GPIOB,15

#define LED_GREEN     GPIOE,2
#define LED_RED       GPIOD,4

#define D1RTE         GPIOA,4
#define D2RTE         GPIOE,10
#define D3RTE         GPIOE,13
#define D4RTE         GPIOB,14

#define D1DPOS        GPIOE,9
#define D2DPOS        GPIOE,12
#define D3DPOS        GPIOE,11
#define D4DPOS        GPIOA,12

#define ETH_CS        GPIOC,6 // pin 9 on J2, used for RS485 EN
#define X_RS485EN     ETH_CS

#define FLASH_CS1     GPIOD,15
#define FLASH_PROTECT GPIOC,8
#define FLASH_RESET   GPIOC,7

#define EECS1         GPIOD,10
#define EECS2         GPIOD,9
#define EECS3         GPIOC,13
#define EECS4         GPIOE,6

#define VDROP         GPIOC,4

extern u16  event_index, event_counter;
extern u16  event_report_index, event_report_count;
extern u16  event_not_sent, event_next_send;

extern u16  clear_apb_table;
extern u8 fire_condition[MAX_DOORS];

extern u8 enable_reader[MAX_DOORS];

extern u8 last_key_to[MAX_DOORS];
extern u32 prev_key[MAX_DOORS];
extern u32 new_key[MAX_DOORS];
extern u8 UserKeyReady[MAX_DOORS];

extern u8 APB_table[MAX_KEY_RECORDS / 4];

extern u32 keys_in_eeprom;

extern u16 waiting_events;

extern u16 reload_door_setup;

extern u16 Ethernet_active;

extern u8 update_locals_in_eeprom;

extern u8 event_log[256];
extern u8 elx;

extern u8 update_internal_setup[MAX_DOORS];

extern u8 mul_t_lock;

extern u16 auto_unlock_act, auto_unlock_ctr, auto_unlock_idx;
extern u32 auto_unlock_mask;

extern SETUP_RECORD global_setup;

extern DOOR_RECORD doors[MAX_DOORS];
extern u16 auto_unlocked[MAX_DOORS];


extern u16 two_man_count[MAX_DOORS];
extern u16  two_man_timer[MAX_DOORS];
extern u32 two_man_first[MAX_DOORS];
extern u16  enable_first_user[MAX_DOORS];

extern u16  delayed_open_timer[MAX_DOORS];
extern u16  delayed_open_state[MAX_DOORS];

extern u16 interlock_disable_unlock[MAX_DOORS];

extern u16  ajar_timeout[MAX_DOORS];
extern u16  door_ajar[MAX_DOORS];
extern u16  check_ajar[MAX_DOORS];
extern u16  unlock10[ELEVATOR_DOORS];

extern u16  DPOS_levels[MAX_DOORS];
extern u16  DPOS_status[MAX_DOORS];
extern u16  debounce_dpos[MAX_DOORS];

extern u16  RTE_status[MAX_DOORS];
extern u16  RTE_pressed[MAX_DOORS];
extern u16  debounce_rte[MAX_DOORS];

extern u16  door_closed[MAX_DOORS];
extern u8 door_change_1[MAX_DOORS];

extern u16 aux_pattern[MAX_DOORS], aux_mask[MAX_DOORS], aux_timer[MAX_DOORS];

extern u16 scan_locks;

extern u16 MyAddress;

extern u16 allow_write;

extern u16 battery_ok;
extern u16 check_battery;
extern u16 power_ok;

extern __no_init u32 count_all_key;


extern u32 fire_detected, send_fire_cancel, fire_debounce;

extern u16 tamper_detected, tamper_debounce;

extern u16 safe_door_mode;

extern u32 eeprom_write_timer;

//============ pin code processing =========================
extern KEY_RECORD pin_key_rec[MAX_DOORS];
extern u32 collected_pin[MAX_DOORS];
extern u16  collected_digits[MAX_DOORS];
extern u16  wait_wiegand_digit[MAX_DOORS];
extern u32 pin_index[MAX_DOORS];


extern u16 COM1_rcnt;
extern u16 COM1_rxi;
extern u16 COM1_rxo;
extern u16 COM1_rbuf[COM1_RX_LEN];
extern u8 comm_tbuf[COM1_TX_LEN];
extern u8 comm_buf[COM1_TX_LEN];
extern u8 comm_echo[COM1_TX_LEN];

extern u16 comm_timer;
extern u16 packet_idx;
extern u16 comm_state;

extern u16 ETH_detected;

//====== date/time =========================================
extern DATE_TIME sys_date_time;
extern u8 day_of_week;

//u8 const max_day[13] = {  0 ,31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
extern const u16 CummulativeMonthDays[12];

//=========== timer =======================================-
extern u32 Timer_1mS_Flags, Timer_1ms_counter;
extern u32 Timer_5mS_Flags;
extern u32 Timer_10mS_Flags, Timer_10mS_Cnt;
extern u32 Timer_100mS_Flags, Timer_100mS_Flags2, Timer_1sec_Flags;
extern u32 Timer_1mS_Count[MAX_DOORS],  Timer_10ms_count[MAX_DOORS]; // used for timeout purposes by handler functions



extern u32 set_1sec_flags;


extern u8 KeySource[MAX_DOORS];

extern u8  rdr_interval[MAX_DOORS];
extern u8  ColumnParity[MAX_DOORS];
extern u8  RDRstate0, RDRstate1;
extern u32  BitCnt[MAX_DOORS];
extern u8  ReaderFreq[MAX_DOORS];

extern u8 reader_bitmask[MAX_DOORS];
extern u8 reader_byteidx[MAX_DOORS];

extern u8 reader_data[MAX_DOORS][MAX_READER_DATA_LEN];
extern u8 received_key[MAX_DOORS][5];

extern u8 *reader_byteptr[MAX_DOORS];

extern u8 WGND_kepress[MAX_DOORS];

extern u16 rdr4_active;

//----------- MCR data--------------------------------------
extern u8 MCR_T1chr[MAX_DOORS];                  // track I character accumulator
extern bool CardReady[MAX_DOORS];
extern bool TrackOneDetected[MAX_DOORS];
extern bool raw_start[MAX_DOORS];



//======== LED and lock variables ==============================
extern u16  led_period[MAX_DOORS], led_pattern[MAX_DOORS], led_mask[MAX_DOORS];
extern u16  led_mode[MAX_DOORS], led_timer[MAX_DOORS];
extern u16  lock_unlocked[MAX_DOORS];
extern u16  alarm_trouble[MAX_DOORS];

extern u16  buzzer_mode[MAX_DOORS];
extern u16  buzzer_pattern[MAX_DOORS];
extern u16  buzzer_timer[MAX_DOORS];
extern u16  buzzer_mask[MAX_DOORS];

extern KEY_RECORD post_key;
extern u32 recv_keys, last_function;
extern u16 event_detected, ack_detected;
extern u16 analog_values[11];


#define EE_LKC 1
#define EE_LKR 2
#define EE_LAL 3
#define EE_LWZ 4
#define EE_LDZ 5
#define EE_LMO 6
#define EE_LEV 7
#define EE_IEV 8
#define EE_LDR1 9
#define EE_LDR2 10
#define EE_TST 11
#define EE_CEA 12
#define EE_IEE 13
#define EE_GST 14
#define EE_EPT 15

typedef enum {
  BZMODE_OFF     =   0,
  BZMODE_ON      =   1,
  BZMODE_TROUBLE =   2,
  BZMODE_ALARM   =   3,
  BZMODE_4_BEEPS =   4,
  BZMODE_1MINUTE =   5
} BZ_MODES;

//-------- messages --------------------------------------------------------
#define MSG_SDTM 0x11 // set date time
#define MSG_ERAU 0x12 // erase all users
#define MSG_ENUL 0x13 // enable global unlock
#define MSG_DSUL 0x14 // disable global unlock
#define MSG_UNLK 0x15 // remote unlock
#define MSG_SKEY 0x16 // set key block contents
#define MSG_DKEY 0x17 // delete key record
#define MSG_SALV 0x18 // set access level
#define MSG_SDRR 0x1A // set door record
#define MSG_SMHL 0x1B // set month holidays
#define MSG_SSTT 0x1C // send door status
#define MSG_REVT 0x1D // request event record
#define MSG_EVNT 0x1E // send event
#define MSG_ERAV 0x1F // erase events
#define MSG_RQST 0x20 // request status
#define MSG_LPRM 0x21 // local parameter setting
#define MSG_QKEY 0x23 // query key
#define MSG_RKEY 0x25 // return key block
#define MSG_FIRE 0x27 // fire mode on/off
#define MSG_SDZN 0x30 // set day zone
#define MSG_STZN 0x31 // set time zone
#define MSG_AUXR 0x33 // remote aux relay
#define MSG_ERAL 0x41 // immediate erare all keys
#define MSG_NKEY 0x52 // key not found
#define MSG_STST 0x74 // self test in test jig

void generate_event(u8 source, u32 ID, u32 kcode, u8 event);
void write_ext_eeprom(u32 address, u8 *data, u16 len);
void read_ext_eeprom(u8 caller, u32 address, u8 *data, u16 len);
void led_onoff(u16 led, u8 onoff);
u32 DB_search_key(u32 pcode, KEY_RECORD *key) ;
u16 DB_search_empty_slot(u32 pcode) ;
void DB_delete_key(u32 index);
void DB_store_key(KEY_RECORD *key, u32 index);
u16 DB_add_key(KEY_RECORD *key) ;

void DB_store_access_level(u16 index, AL_RECORD *al);
void DB_load_access_level(u16 index, AL_RECORD *al);
void DB_store_weekzone(u16 index, WZ_RECORD *tz);
void DB_load_weekzone(u16 index, WZ_RECORD *tz);
void DB_store_dayzone(u16 index, DZ_RECORD *dz);
void DB_load_dayzone(u16 index, DZ_RECORD *dz);
void DB_store_holidays(u16 month, u8 *holidays);
void DB_load_holidays(u16 month, u8 *holidays);
void DB_erase_Al_wk_dy(void);
void set_apb_counter(u32 index, u16 dir) ;
u8 get_apb_counter(u32 index) ;
u32 count_keys_in_DB(void);
void erase_keys_in_DB(void);
u8 test_door_flag(u8 door, u32 flags);
void init_event(void);
void check_fire_input(void);
void check_tamper(void);
void delay_ms(u32 delay);
void set_buzzer(u32 door, BZ_MODES cmd);
void set_aux(u16 door, u16 pattern);
void door_buzzer(u16 door, u8 onoff);
void execute_unlock(u16 source, u16 Flags);
bool check_disable_conditions(u16 source, u32 key, u32 ID, u16 Flags);
void start_pin_code_wait(u16 source, u32 index, KEY_RECORD *key_rec);
void init_pin_only_wait(u16 source);
bool pin_only_mode(u16 source);
void clear_pin_wait(u16 source);
void process_key(u32 source, u32 key, u16 special);
void delay_cycles(u32 cyc);
void delay_us(u32 dly);
void delay_ms(u32 dly);
void restart_wdt(void);
void MCR_init_magcard(u32 reader);
void door_buzzer(u16 door, u8 onoff);
u8 get_address(void);
void operate_aux(u16 door, u16 onoff);
void operate_lock(u16 door, u8 onoff);
void set_aux(u16 door, u16 pattern);
void handle_aux_relays(void);
void led_onoff(u16 led, u8 onoff);
void set_led(u16 door, u8 cmd);
void led_handler(void);
void relock(u16 door);
void handle_rte(void);
void handle_dpos(void);
void handle_doors(void);
void set_adc_channel(u32 channel);
u32 read_adc(void);
void get_analog_inputs(void);
u16 get_reader_interrupts(u16 reader);
void check_fire_input(void);
void check_tamper(void);
void  unlock_door(u32 lock);
void    update_date_time(void);
u32   date_time_diff(DATE_TIME *time1, DATE_TIME *time2);
void    generate_event(u8 source, u32 ID, u32 key, u8 event);
void    load_mfg_defaults(void);
void    save_date_time(u8 full);
void    PC_send_status(void);
void    PC_send_my_id(void);
void    read_door_setup(u32 door);
void    write_door_setup(u8 door);
void    operate_aux_relay(u8 door, u8 seconds);
//void    process_key(u32 source, u32 key, u16 special);
u8   search_in_invalid_key_list(u32 key);
void    erase_from_invalid_key_list(u32 key);
//void    add_to_invalid_key_list(key);
void    background_tasks(void);
u8   test_door_flag(u8 door, u32 flags);
bool     check_access_level(u32 door, u32 aln);
//void    DB_store_access_level(u8 index, AL_RECORD *al);
//void    DB_store_weekzone(u8 index, WZ_RECORD *tz);
bool     send_event(u16 index, EVENT_RECORD *evt);
void    write_global_setup(void);
bool     locker_matrix_operation(void);
void delay_mst(u16 delay);
void process_incoming_event(u8 *comm_buf);
void clear_pin_wait(u16 source);
void send_fire_alarm(u32 onoff);
void init_pin_only_wait(u16 source);
bool pin_only_mode(u16 source);
void rdr4_red(u32 reader);
void rdr4_green(u32 reader);
void rdr4_yellow(u32 reader);
u16 RTC_ComputeDay(DATE_TIME *dt);
void WGND_init(u8 reader);
void  MX_USART1_UART_Init(void);
void  MX_USART2_UART_Init(void);
u16 CalculateCRC(u8 *BufStart, u16 BufLen);
void wiegand_d0(u32 reader);
void wiegand_d1(u32 reader);
void MCR_MagData(u32 reader);
void delay_cycles(u32 cyc);
void IIC_Delay(void);
void  init_IIC(void);
void  IIC_Write_Cycle(void);
bool IIC_Write(u8  send_bits);
u8 IIC_ReadByte(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_SendAck(void);
void IIC_SendOne(void);
void enable_ext_eeprom_write(void);
void disable_ext_eeprom_write(void);
void wait_eeprom_write(u8 select);
void write_ext_eeprom(u32 address, u8 *data, u16 len);
void read_ext_eeprom(u8 caller, u32 address, u8 *data, u16 len);
bool check_eeprom_address(void);
void init_eeprom(void);
void RTC_store_date_time(DATE_TIME *idt);
void RTC_load_date_time(void);
void ETHR_send_char(u8 chr);
void RS485_send_char(u8 chr);
bool check_time_zone(u16 tzn);
u8 MCR_wait_magcard(u32 reader);
u8 WGND_wait(u32 reader);
void GKP_comm_handler(void);
u16 check_event_index_validity(void);
void get_event_record(u32 index, EVENT_RECORD *evt);
void erase_events(void);
void long_to_date(u32 compdt, DATE_TIME *dt);
void RTC_init_chip(void);
void COM1_init(void);
void reset_lantronix(void);
void COM1_send_string(u8 *bp);
bool COM1_send_packet(u8 dest, u8 source, u32 length);
void flush_rx_buffer(u32 channel);
void buzzer_handler(void);
void handle_keys(void);
void battery_handler(void);
void put_short(u8 dat);





#define ANALOG_FIRE 0
#define ANALOG_TAMPER 1
#define ANALOG_SINGLE_DOOR 2
#define ANALOG_RDR4 3





#define LMODE_OFF         0
#define LMODE_ON          8
#define LMODE_SLOW       16
#define LMODE_FAST       24
#define LMODE_BLINK_4    32

#include "DLib_Product_string.h"

#endif

extern const u32 Tmr_1mS_DELAY          ; //  1
extern const u32 Tmr_1mS_SEND_EVENT     ; //  2
extern const u32 Tmr_1mS_SCAN           ; //  8
extern const u32 Tmr_1mS_KEYS           ; // 16
extern const u32 Tmr_1mS_COMM_PACING     ; // 64
extern const u32 Tmr_1mS_CHG_BATTERY    ; //128
extern const u32 Tmr_1mS_KEY_DELAY      ; //256

extern const u32 Tmr_5mS_RTE            ; //  1
extern const u32 Tmr_5mS_DPOS           ; //  2
extern const u32 Tmr_5mS_IOX_PRESENT    ; //  4
extern const u32 Tmr_5mS_DELAYED_OPEN   ; //  8
extern const u32 Tmr_5mS_TOGGLE_LED     ; // 16

extern const u32 Tmr_10mS_COMM_TIMER    ; //  1
extern const u32 Tmr_10mS_RDR_LEDS      ; //  2
extern const u32 Tmr_10mS_READER0       ; //  4
extern const u32 Tmr_10mS_LEDS0         ; //  8
extern const u32 Tmr_10mS_READER1       ; // 16
extern const u32 Tmr_10mS_LEDS1         ; // 32
extern const u32 Tmr_10mS_INT_BUZZER    ; // 64
extern const u32 Tmr_10mS_RDR4_TIME     ; //128

extern const u32 Tmr_100mS_CLEAR_APB    ; //  1
extern const u32 Tmr_100mS_KEYS         ; //  2
extern const u32 Tmr_100mS_UPDT         ; //  4
extern const u32 Tmr_100mS_RDR_LEDS     ; //  8
extern const u32 Tmr_100mS_SCAN         ; // 16
extern const u32 Tmr_100mS_AUTOLOCK     ; // 32
extern const u32 Tmr_100mS_RDR_BUZZERS  ; // 64
extern const u32 Tmr_100mS_NO_COMM      ; //128
extern const u32 Tmr_100mS_SAMPLE       ; //256
extern const u32 Tmr_100mS_AUX_RELAYS   ; //512

extern const u32 Tmr_1sec_AUTOLOCK      ; //  1
extern const u32 Tmr_1sec_DOOR_AJAR     ; //  2
extern const u32 Tmr_1sec_AUX_RELAYS    ; //  4
extern const u32 Tmr_1sec_CHG_BATTERY   ; //  8
extern const u32 Tmr_1sec_Elev_Reset    ; // 16
