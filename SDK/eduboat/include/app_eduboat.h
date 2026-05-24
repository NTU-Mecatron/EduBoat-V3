#ifndef _APP_EDUBOAT
#define _APP_EDUBOAT

#include <Arduino.h>
#include <stdint.h>
#include <MsgPack.h>

#define MIN_SPEED 1100
#define MIN_ALLOWABLE_SPEED 1350
#define MAX_SPEED 1900
#define MAX_ALLOWABLE_SPEED 1650
#define STOP_SPEED 1500

#define AP_CHANNEL 1

static struct Message
{
    uint8_t x;
    uint8_t y;

    MSGPACK_DEFINE(x, y);
} message;

typedef struct reqval_t
{
    uint16_t target_val;
    uint16_t curr_val;
    uint16_t min_val;
    uint16_t no_val;
    uint16_t max_val;
    uint16_t rateOfChange;
} reqval_t;

#endif /* _APP_EDUBOAT */
