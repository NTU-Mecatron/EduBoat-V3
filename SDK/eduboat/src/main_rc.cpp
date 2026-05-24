#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

#include "app_eduboat.h"

#define ROV_RC 1
#include "board_defs.h"

// configurables ===================================================================

#define JOY_X_PIN PIN_ANA_01
#define JOY_Y_PIN PIN_ANA_02

//  ================================================================================

uint8_t boatID = 0xFF;

WiFiUDP udp;
Coap coap(udp);
MsgPack::Packer packer;
char AP_SSID[16];
char AP_PASSWORD[16];

hw_timer_t *timer = NULL;

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

void send_values()
{
    static uint16_t x = analogRead(JOY_X_PIN);         // Read joystick Y-axis value
    static uint16_t y = analogRead(JOY_Y_PIN);         // Read joystick X-axis value
    message.x = map(x, 0, 4095, 0, 255); // Map to 0-255 range
    message.y = map(y, 0, 4095, 0, 255); // Map to 0-255 range
    
    IPAddress server_ip(192, 168, boatID, 1);
    packer.clear();
    packer.serialize(message);
    coap.put(server_ip, 5683, "message", (const char *)packer.data(), packer.size());
}

// ===== TIMER ===== //
void IRAM_ATTR onTimer()
{
    send_values();
}

/**
 * @brief setup the timer interrupt to call update_values() at a fixed interval (10Hz).
 * 
 */
void setupTimer()
{
    timer = timerBegin(0, 80, true); // 1µs tick
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 20000, true); // 20ms
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
WiFi.begin(AP_SSID, AP_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to AP");
}

void setup()
{
    Serial.begin(115200);

    setBoatID();
    setWifi();

    setupTimer();
}

void loop()
{
    delay(25);
    // coap.loop();
}
