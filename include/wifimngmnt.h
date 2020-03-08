#ifndef WIFIMNGMNT_H_
#define WIFIMNGMNT_H_

#include <Arduino.h>

#define REDLED 13
#define GREENLED 15

typedef enum {
    NOCON,
    WIFIWS
} con_mode_t;

String printWifiScan(void);
void comconfigure(void);
void loopmanager(void);
void luosloop(void);
bool getappoint(void);

#endif //WIFIMNGMNT_H_
