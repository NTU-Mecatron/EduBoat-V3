#ifndef _BOARD_DEFS
#define _BOARD_DEFS

#include <stdint.h>

#if ROV_RC
    const uint8_t BOATIDSEL_PINS[] = {14, 13, 12, 11, 10, 9};
    #define PIN_SW_1 (47)
    #define PIN_SW_2 (48)
    #define PIN_SW_3 (35)
    #define PIN_SW_4 (36)
    #define PIN_SW_5 (37)
    #define PIN_SW_6 (38)
    #define PIN_SW_7 (39)
    #define PIN_SW_8 (40)
    #define PIN_SW_9 (41)
    #define PIN_SW_10 (42)
    #define PIN_SW_11 (01)
    #define PIN_SW_12 (02)

    #define PIN_PWM_01 (19)
    #define PIN_PWM_02 (08)
    #define PIN_PWM_03 (18)
    #define PIN_PWM_04 (17)

    #define PIN_ANA_01 (16)
    #define PIN_ANA_02 (15)
    #define PIN_ANA_03 (07)
    #define PIN_ANA_04 (06)
    #define PIN_ANA_05 (05)
    #define PIN_ANA_06 (04)
#endif

#if EB1_RC
    const uint8_t BOATIDSEL_PINS[] = {1, 2, 42, 41, 40 ,39};
    #define PIN_JOY_L_X (12)
    #define PIN_JOY_L_Y (13)
    #define PIN_JOY_L_Z (14)
    #define PIN_JOY_R_X (18)
    #define PIN_JOY_R_Y (17)
    #define PIN_JOY_R_Z (16)
#endif

#if EB1_PDUFC
    const uint8_t BOATIDSEL_PINS[] = {1, 2, 42, 41, 40 ,39};
    #define PIN_PWM_01 (14)
    #define PIN_PWM_02 (13)
    #define PIN_PWM_03 (12)
    #define PIN_PWM_04 (11)
    #define PIN_PWM_05 (10)
    #define PIN_PWM_06 (9)
#endif

#endif /* _BOARD_DEFS */
