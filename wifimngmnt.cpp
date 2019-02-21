#include "wifimngmnt.h"
#include "config.h"
#include "webpage.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <BluetoothSerial.h>
#include <HardwareSerial.h>

// Wifi parameters
WiFiMulti wifiMulti;
WebSocketsServer webSocket = WebSocketsServer(WSPORT);
char hostString[HOSTNAMESIZE] = {0};

// DNS server parameters
IPAddress apIP(192, 168, 1, 1);
const byte DNS_PORT = 53;
DNSServer dnsServer;
con_mode_t con_mode = NOCON;

// Bluetooth
BluetoothSerial SerialBT;

// led things
bool appoint = true;

void (*cb)(uint8_t, uint8_t*, size_t);

// Bluetooth event callback
void bleEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
    if(event == ESP_SPP_SRV_OPEN_EVT){
        Serial.println("Bluetooth client Connected");
        con_mode = BLESERIAL;
        digitalWrite (GREENLED, HIGH);
    }
    if(event == ESP_SPP_CLOSE_EVT){
        Serial.println("Bluetooth client Disconnected");
        con_mode = NOCON;
        digitalWrite (GREENLED, LOW);
    }
}

bool getappoint(void) {
  return appoint;
}

// Websocket event callback
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("Websocket disconnected!\n");
            con_mode = NOCON;
            // Turn off green led
            digitalWrite (GREENLED, LOW);
        break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("Websocket connected from %d.%d.%d.%d url: %s\n", ip[0], ip[1], ip[2], ip[3], payload);
                con_mode = WIFIWS;
                // Turn on green LED
                digitalWrite (GREENLED, HIGH);
            }
        break;
        case WStype_TEXT:
            {
              uint8_t datas[lenght +1];
              memcpy(datas, payload, lenght);
              // add a '\r' at the end of the string because L0 need it to detect the end of the Json
              datas[lenght] = '\r';
              // Send those datas to L0
              Serial2.write(datas, lenght+1);
            }
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary lenght: %u\n", num, lenght);
        break;
        default:
        break;
    }
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
    char jsonString[512] = {0};
    int i = 0;
    while(Serial2.available()) {
        jsonString[i++] = char(Serial2.read());
    }
    if (jsonString[0] != 0) {
        // search the first '\n' of the message
        for (i = 0; i < strlen(jsonString); i++) {
            if(jsonString[i] == '\n') break;
        }
        // check errors
        if (i == strlen(jsonString)) return;
        // Finish the String just to make sure
        jsonString[i+1] = '\0';
        // transfert Json to the current wireless communication way
        if (con_mode == WIFIWS) {
            webSocket.sendTXT(0, jsonString);
        }
        if (con_mode == BLESERIAL) {
            SerialBT.write((const uint8_t*)jsonString, strlen(jsonString));
        }
    }
}

// manage messages received from bluetooth
void bleloop(void) {
    static char jsonString[256] = {0};
    while (SerialBT.available()) {
        const char letter = char(SerialBT.read());
        sprintf(jsonString, "%s%c", jsonString, letter);
        if (letter == '\r') {
            Serial2.print(jsonString);
            jsonString[0] = '\0';
        }
    }
}

// The function who call all loops who need to be updated
void loopmanager(void) {
    webSocket.loop();
    ArduinoOTA.handle();
    pageloop();
    if (appoint & (con_mode == NOCON)) {
      dnsServer.processNextRequest();
    }
    luosloop();
    bleloop();
}

//Connect to the EEPROM saved wifi, else create access point
void wifi_connect() {
    Serial.println("");
    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        // Stop led blink 
        appoint = false;
        // Stop captive portal
        dnsServer.stop();
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
        // configure the captive portal
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", apIP);
        Serial.println("captive portal started");
    }
}

// Setup the entire communication ways
void comconfigure() {
    // Setup serial to L0
    Serial2.begin(1000000, SERIAL_8N1, 16, 17);    //Baud rate, parity mode, RX, TX
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
    // Open webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // Setup web server
    pageinit();

    // Start OTA server.
    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname(hostString);
    ArduinoOTA.begin();

    //Add services to MDNS allowing this module to be easily detected in the network
    MDNS.addService("luos", "tcp", LUOSDETECT);
    MDNS.addService("http", "tcp", SERVERPORT);
    MDNS.addService("ws", "tcp", WSPORT);

    //Bluetooth configurration
    SerialBT.register_callback(bleEvent);
    SerialBT.begin(hostString); //Bluetooth device name
    Serial.print("Bluetooth started as ");
    Serial.println(hostString);
}
