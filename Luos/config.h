#ifndef CONFIG_H_
#define CONFIG_H_


// Default configurations
#define HOSTNAME              "LuosNetwork"

// PINOUT LEDS
#define REDLED                13
#define GREENLED              15

// Upload Mode
#define APP_UPLOAD_SERIAL     0
#define APP_UPLOAD_OTA        1

// Ports assignation
#define SERVERPORT            80
#define LUOSDETECT            9010
#define TCP_PORT              9342

// Page style
#include <pgmspace.h>
const char PAGESTYLE[] PROGMEM = "<style>.c{text-align: center;} input[type=\"text\"]{width:45%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#34a3b4;color:#fff;line-height:2.4rem;font-size:1.2rem;width:50%;} .q{float: right;width: 64px;text-align: right;} </style>";


/************************* EEPROM address and size *******************************/

#define HOSTNAMEADD           0
#define HOSTNAMESIZE          30
#define SSIDADD               HOSTNAMESIZE
#define SSIDSIZE              40
#define PASSADD               HOSTNAMESIZE + SSIDADD
#define PASSSIZE              40

/************************* TCP *******************************/
#define MAX_CLIENTS           1

/*************************  COM Port 0 *******************************/
#define UART_DEBUG_BAUD       115200        // Baudrate UART0
#define SERIAL_DEBUG_PARAM    SERIAL_8N1    // Data/Parity/Stop UART0
#define SERIAL_DEBUG_RXPIN    3             // receive Pin UART0
#define SERIAL_DEBUG_TXPIN    1             // transmit Pin UART0
/*************************  COM Port 1 *******************************/
#define UART_LUOS_BAUD        1000000       // Baudrate UART1
#define SERIAL_LUOS_PARAM     SERIAL_8N1    // Data/Parity/Stop UART1
#define SERIAL_LUOS_RXPIN     16            // receive Pin UART1
#define SERIAL_LUOS_TXPIN     17            // transmit Pin UART1

#define SERIAL_BUFFER_SIZE    4096

#endif //CONFIG_H_
