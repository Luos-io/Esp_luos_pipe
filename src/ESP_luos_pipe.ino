#include <Arduino.h>
#include <config.h>

/*
 *  This sketch try to Connect to the best AP based on a given list
 *
 */

#include "wifimngmnt.h"


void setup() {
    // Setup serial debug output
    Serial.begin(115200, SERIAL_8N1, 3, 1);
    Serial.setRxBufferSize(SERIAL_BUFFER_SIZE);
    
    Serial.begin(115200);
    Serial.setDebugOutput(true);
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
