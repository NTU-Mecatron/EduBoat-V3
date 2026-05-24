#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ESP32Servo.h>
#include "ESC.h"

#include "app_eduboat.h"

#define EB1_PDUFC 1
#include "board_defs.h"

// configurables ===================================================================

const uint8_t rateOfChange = 10;
const uint8_t deadzone_center = 15;
const uint8_t deadzone_ramp = 10;
const uint16_t msg_timeout = 500;

//  ================================================================================

reqval_t reqval_L;
reqval_t reqval_R;

ESC thrusterL(PIN_PWM_01, MIN_SPEED, MAX_SPEED, STOP_SPEED);
ESC thrusterR(PIN_PWM_02, MIN_SPEED, MAX_SPEED, STOP_SPEED);

uint8_t boatID = 0xFF;

WiFiUDP udp;
Coap coap(udp);
MsgPack::Unpacker unpacker;
char AP_SSID[16];
char AP_PASSWORD[16];

hw_timer_t *timer = NULL;
unsigned long lastCbTime = 0;
bool timedout = true;

/**
 * @brief Set the Boat ID
 * 
 */
void setBoatID()
{
    for (int i = 0; i < 6; i++)
    {
        pinMode(BOATIDSEL_PINS[i], INPUT_PULLUP);
        digitalRead(BOATIDSEL_PINS[i]) ? boatID |= (1 << i) : boatID &= ~(1 << i);
    }
}

/**
 * @brief Calculate the next PWM value for an ESC based on the target value, current value, and rate of change. \
 * Implements a deadzone around the stop signal to prevent oscillation, and ramps towards the target value at a fixed rate.
 * 
 * @param reqval 
 * @return true - PWM value updated and ESC should be commanded
 * @return false - No update needed (within deadzone), ESC can keep current speed
 */
bool calc_ramp_reqval(reqval_t *reqval)
{
    // if near center, set to stop immediately
    if (abs(reqval->target_val - STOP_SPEED) < deadzone_center)
    {
        reqval->curr_val = reqval->target_val = STOP_SPEED;
    }
    // else if far from target, ramp by rateOfChange
    else if (abs(reqval->curr_val - reqval->target_val) > deadzone_ramp)
    {
        reqval->curr_val +=
            (reqval->curr_val > reqval->target_val)
                ? -reqval->rateOfChange
                : reqval->rateOfChange;

        reqval->curr_val = constrain(
            reqval->curr_val,
            reqval->min_val,
            reqval->max_val);
    }
    // if within deadzone, return false to indicate no update needed
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief Calculate differential thrust for left and right thrusters based on joystick inputs.
 * 
 * @param x (0-255) Joystick X-axis value, where 128 is centered. Mapped to turning command.
 * @param y (0-255) Joystick Y-axis value, where 128 is centered. Mapped to forward/backward command.
 * @param reqval_L 
 * @param reqval_R 
 */
void calc_differential_thrust(int16_t x, int16_t y, reqval_t *reqval_L, reqval_t *reqval_R)
{
    static int16_t forward = map(y, 0, 255, -MAX_ALLOWABLE_SPEED, MAX_ALLOWABLE_SPEED);
    static int16_t turn = map(x, 0, 255, -MAX_ALLOWABLE_SPEED, MAX_ALLOWABLE_SPEED);
    static int16_t left_target = forward + turn;
    static int16_t right_target = forward - turn;

    reqval_L->target_val = left_target;
    reqval_R->target_val = right_target;
}

/**
 * @brief check if the last received command has timed out.
 * 
 * @return true 
 * @return false 
 */
bool check_timeout()
{
    timedout = (millis() - lastCbTime) > msg_timeout;
    return timedout;
}

/**
 * @brief Update the PWM values for the thrusters based on the current target values.
 * 
 */
void update_values()
{
    if (check_timeout())
    {
        reqval_L.target_val = reqval_L.no_val;
        reqval_R.target_val = reqval_R.no_val;
    }

    if (calc_ramp_reqval(&reqval_L))
        thrusterL.speed(reqval_L.curr_val);
    if (calc_ramp_reqval(&reqval_R))
        thrusterR.speed(reqval_R.curr_val);

    thrusterL.speed(reqval_L.curr_val);
    thrusterR.speed(reqval_R.curr_val);
}

/**
 * @brief Handle incoming messages and update thruster values accordingly.
 * 
 */
void handle_message()
{
    timedout = false;
    lastCbTime = millis();
    static uint16_t x = message.x;
    static uint16_t y = message.y;
    calc_differential_thrust(x, y, &reqval_L, &reqval_R);
}

/**
 * @brief coap message handler - called when a message is received. Parses the message and updates the target values for the thrusters.
 * 
 * @param packet 
 * @param ip 
 * @param port 
 */
void handle_coap_msg(CoapPacket &packet, IPAddress ip, int port)
{
    try
    {
        unpacker.feed(packet.payload, packet.payloadlen);
        unpacker.deserialize(message);
        // Serial.printf("Received - x: %d, y: %d\r\n", message.x, message.y);

        handle_message();
    }
    catch (...)
    {
        Serial.println("Invalid data!");
        coap.sendResponse(ip, port, packet.messageid, "ERR");
    }
}

// ===== TIMER ===== //
void IRAM_ATTR onTimer()
{
    update_values();
}

/**
 * @brief setup the timer interrupt to call update_values() at a fixed interval (10Hz).
 * 
 */
void setupTimer()
{
    timer = timerBegin(0, 80, true); // 1µs tick
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 100000, true); // 100ms
    timerAlarmEnable(timer);
}

/**
 * @brief setup the WiFi access point.
 * 
 */
void setWifi()
{
    snprintf(AP_SSID, sizeof(AP_SSID), "boat%03d", boatID);
    snprintf(AP_PASSWORD, sizeof(AP_PASSWORD), "%02d%02d%02d%02d",
             boatID, boatID, boatID, boatID);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, boatID, 1),
                      IPAddress(192, 168, boatID, 1),
                      IPAddress(255, 255, 255, 0));

    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);

    Serial.print("AP SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}

void setup()
{
    Serial.begin(115200);

    reqval_L = {STOP_SPEED, STOP_SPEED, MIN_ALLOWABLE_SPEED, STOP_SPEED, MAX_ALLOWABLE_SPEED, rateOfChange};
    reqval_R = {STOP_SPEED, STOP_SPEED, MIN_ALLOWABLE_SPEED, STOP_SPEED, MAX_ALLOWABLE_SPEED, rateOfChange};

    thrusterL.arm();
    thrusterR.arm();
    delay(2000);
    thrusterL.speed(STOP_SPEED);
    thrusterR.speed(STOP_SPEED);
    delay(1000);

    setBoatID();
    setWifi();
    coap.server(handle_coap_msg, "message");
    coap.start();

    setupTimer();
}

void loop()
{
    coap.loop();
}
