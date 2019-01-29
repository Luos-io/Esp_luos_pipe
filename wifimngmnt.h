#ifndef WIFIMNGMNT_H_
#define WIFIMNGMNT_H_

#include <Arduino.h>

typedef enum {
    NOCON,
    BLESERIAL,
    WIFIWS
} con_mode_t;

String printWifiScan(void);
void comconfigure(void);
void loopmanager(void);
void luosloop(void);

#endif //WIFIMNGMNT_H_
