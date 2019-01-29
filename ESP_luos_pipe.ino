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
    // Configure all communication ways
    comconfigure();
}

void loop()
{
    loopmanager();
}
