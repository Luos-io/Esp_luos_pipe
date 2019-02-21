#define SERIAL_BUFFER_SIZE 512
#include <Arduino.h>

// Bluetooth compile warning
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


/*
 *  This sketch try to Connect to the best AP based on a given list
 *
 */

#include "wifimngmnt.h"


void setup() {
    // Setup serial debug output
    Serial.begin(115200);
    pinMode (GREENLED, OUTPUT);
    pinMode (REDLED, OUTPUT);
    digitalWrite (GREENLED, LOW);
    // Red led blink in hotspot mode and stay on if connected to a wifi
    digitalWrite (REDLED, LOW);
    // Configure all communication ways
    comconfigure();

    //Setup leds
    digitalWrite (REDLED, HIGH);
}

void loop()
{
    static unsigned long lastblink = millis();
    if (getappoint() & (millis() - lastblink > 1000)) {
      digitalWrite (REDLED, !digitalRead(REDLED));
      lastblink = millis();
    }
    loopmanager();
}
