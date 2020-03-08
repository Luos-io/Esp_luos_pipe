#include "wifimngmnt.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <EEPROM.h>

// Wifi parameters
WiFiMulti wifiMulti;
AsyncClient* moClient;
AsyncServer* moServer;
char hostString[HOSTNAMESIZE] = {0};

// DNS server parameters
IPAddress apIP(192, 168, 1, 1);
const byte DNS_PORT = 53;
con_mode_t con_mode = NOCON;

// Serial connection
#define SERIAL_LUOS     Serial_Luos
HardwareSerial SERIAL_LUOS(1);

// led things
bool appoint = true;

void (*cb)(uint8_t, uint8_t*, size_t);

bool getappoint(void) {
  return appoint;
}

/* Clients events */
static void handleError(void* arg, AsyncClient* poClient, int8_t error) {
  Serial.printf("TCP Client : handleError\n");
}

// TCP to Serial
static void handleData(void* arg, AsyncClient* poClient, void *data, size_t len) {
    Serial.write((uint8_t*)data, len);
	SERIAL_LUOS.write((uint8_t*)data, len);
}

static void handleDisconnect(void* arg, AsyncClient* poClient) {
  Serial.printf("TCP Client : handleDisconnect\n");
  con_mode = NOCON;
  // Turn off green led
  digitalWrite (GREENLED, LOW);
}

static void handleTimeOut(void* arg, AsyncClient* poClient, uint32_t time) {
  Serial.printf("TCP Client : handleTimeOut\n");
}

/* Server events */
static void handleNewClient(void* arg, AsyncClient* poClient) {
    // Add to list
    moClient = poClient;

    poClient->setNoDelay(true);

    // Register events
    poClient->onData(&handleData, NULL);
    poClient->onError(&handleError, NULL);
    poClient->onDisconnect(&handleDisconnect, NULL);
    poClient->onTimeout(&handleTimeOut, NULL);

    Serial.printf("TCP Server : New client!\n");
    con_mode = WIFIWS;
    // Turn on green LED
    digitalWrite (GREENLED, HIGH);
}

// Only for debug purpose, print wifi list into debug serial
String printWifiScan(void) {
    // WiFi.scanNetworks will return the number of networks found
    String result = "\nWIFI scan...\n";
    char temp [100];
    int n = WiFi.scanNetworks();
    if (n == 0) {
        result += "no networks found\n";
    } else {
        sprintf(temp, "=> %d networks found\n", n);
        result += temp;
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            sprintf(temp, "%d: ", i + 1);
            result += temp;
            result += WiFi.SSID(i);
            sprintf(temp, " (%d)", WiFi.RSSI(i));
            result += temp;
            result += ((WiFi.encryptionType(i) == 7)?"\n":"*\n");
            delay(10);
        }
    }
    return result;
}

// manage messages received from L0 and send them to the current output
void luosloop() {
    static char jsonString[BUFFERSIZE] = {0};
    static int i = 0;
    int msg_end = 0;
    while(SERIAL_LUOS.available()) {
        jsonString[i] = char(SERIAL_LUOS.read());
        if (jsonString[i] == '\n') {
          msg_end = i;
          i++;
          if (i > BUFFERSIZE) Serial.println("serial buffer overflow ! ");
          break;
        }
        i++;
        if (i > BUFFERSIZE) Serial.println("serial buffer overflow ! ");
    }
    if ((msg_end != 0)) {
        if (con_mode == WIFIWS) {

            static unsigned long in_life = millis();
            static int count = 0;
            static int failure = 0;
            // Finish the String just to make sure */
            jsonString[i] = '\0';
            // transfert Json to the current wireless communication way
            moClient->add((char*)jsonString, strlen(jsonString));
            while(!moClient->canSend());
            failure += !moClient->send();
            if (millis() - in_life > 10000) {
                Serial.printf("Still living no %f => state %d => failure %d : %s", count/6.0, moClient->state(),failure,  jsonString);
                count++;
                in_life = millis();
            }
        }
        // clean data
        i = 0;
        jsonString[i] = '\0';
    }
}

// The function who call all loops who need to be updated
void loopmanager(void) {
    luosloop();
}

//Connect to the EEPROM saved wifi, else create access point
void wifi_connect() {
    Serial.println("");
    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        // Stop led blink 
        appoint = false;
        // Print connection details
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("no wifi connection found, starting access point...");
        // Start wifi AP
        WiFi.mode(WIFI_AP_STA);
        // Use 192.168.1.1 fixed IP
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        // Use hostname as AP name
        WiFi.softAP(hostString);
        Serial.print("AP IP address: ");
        Serial.println(apIP);
    }
}

// Setup the entire communication ways
void comconfigure() {
    // Setup serial to L0
    SERIAL_LUOS.begin(1000000, SERIAL_8N1, 16, 17);    //Baud rate, parity mode, RX, TX
    SERIAL_LUOS.setRxBufferSize(BUFFERSIZE);
    // Start EEPROM
    EEPROM.begin(512);
    // Retrive SSID and password
    char ssid[SSIDSIZE] = {0};;
    char pass[PASSSIZE] = {0};;
    for (int i=0; i<SSIDSIZE; i++) {
        ssid[i] = EEPROM.read(SSIDADD+i);
    }
    for (int i=0; i<PASSSIZE; i++) {
        pass[i] = EEPROM.read(PASSADD+i);
    }
    // add the SSID and pass to known wifi
    wifiMulti.addAP(ssid, pass);

    // Retrive hostname
    if(EEPROM.read(HOSTNAMEADD) == 0xAA) {
        for (int i=1; i<=HOSTNAMESIZE; i++) {
            hostString[i-1] = EEPROM.read(HOSTNAMEADD+i);
        }
    }
    else {
        sprintf(hostString, HOSTNAME);
    }
    Serial.print("Hostname: ");
    Serial.println(hostString);
    // Set hostname into wifi configuration
    WiFi.setHostname(hostString);
    // Print list of wifi for debug
    Serial.println(printWifiScan());
    // Try to connect to the saved SSID or create AP
    wifi_connect();

    // Start Bonjour/Avahi/MDNS protocol
    if(MDNS.begin(hostString)) {
        Serial.println("MDNS responder started");
    }

    //Add services to MDNS allowing this module to be easily detected in the network
    MDNS.addService("luos", "tcp", LUOSDETECT);
    MDNS.addService("http", "tcp", SERVERPORT);
    MDNS.addService("stream", "tcp", TCP_PORT);

    // Start listening on TCP port
	moServer = new AsyncServer(TCP_PORT);
	moServer->onClient(&handleNewClient, moServer);
	moServer->begin();
}
