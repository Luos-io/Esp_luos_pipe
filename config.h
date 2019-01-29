#ifndef CONFIG_H_
#define CONFIG_H_

// Default configurations
#define HOSTNAME "LuosNetwork"
#define WSPORT 9342
#define SERVERPORT 80
#define LUOSDETECT 9010

// EEPROM address and size
#define HOSTNAMEADD 0
#define HOSTNAMESIZE 30
#define SSIDADD HOSTNAMESIZE
#define SSIDSIZE 40
#define PASSADD HOSTNAMESIZE + SSIDADD
#define PASSSIZE 40

#endif //CONFIG_H_
