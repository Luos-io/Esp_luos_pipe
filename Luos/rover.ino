#include <Arduino.h>

#include "config.h"
#include <esp_wifi.h>
#include <WiFi.h>

// *** Declaration of serials ***
#define SERIAL_DEBUG    Serial
#define SERIAL_LUOS     Serial_Luos
HardwareSerial SERIAL_LUOS(1);

// Declaration of TCP servers
#include <WiFi.h>
#include <AsyncTCP.h>
#include <DNSServer.h>

AsyncClient* moClient;
AsyncServer* moServer;

// Declaration of buffer for communications
uint8_t cBuffer[SERIAL_BUFFER_SIZE];

// WiFi
#include <ESPmDNS.h>

// WiFiManager
#include <WebServer.h>
#include <WiFiManager.h>

// OTA
#include <Update.h>
uint8_t iOldPercent = 0;
WebServer oWebServer(SERVERPORT);

// EEPROM
#include <EEPROM.h>
char hostString[HOSTNAMESIZE] = {0};
volatile char connected = 0;

// Generics functions
String toStringIP(IPAddress ip) {
  String res = "";
  for (uint8_t iIPPart = 0; iIPPart < 3; iIPPart++) {
    res += String((ip >> (8 * iIPPart)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

// Functions for OTA
void update_start() {
  delay(500);
}

void update_process(size_t current, size_t total) {
  if (total == 0) {
    return;
  }

  uint8_t iPercent = floor(100 * current / total);
  if (iPercent % 10 == 0 && iOldPercent != iPercent) {
    SERIAL_DEBUG.printf("Updating Progress = %d%%\n", iPercent);
    iOldPercent = iPercent;
  }
}

void update_end(uint8_t piRet) {
  if (piRet == 0) {
  } else if (piRet == 1) {
  } else {
  }
}

// Setup configuration communications
void CommunicationConfigure() {

  // WiFi Manager
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.autoConnect(hostString);

  // Start Bonjour/Avahi/MDNS protocol
  if(MDNS.begin(hostString)) {
    SERIAL_DEBUG.printf("MDNS responder started!\n");
  }

  SERIAL_DEBUG.printf("My IP is '%s'\n", toStringIP(WiFi.localIP()).c_str());
  SERIAL_DEBUG.printf("My SSID is '%s'\n", WiFi.SSID().c_str());

  // Webserver configurations
  oWebServer.on("/", HTTP_GET, []() {
    // Page css is stored in flash to save RAM
    String page = FPSTR(PAGESTYLE);
    page += "<h3>Welcome to the ";
    char temp[HOSTNAMESIZE];
    // Check if there is an hostname saved and put it into HTML
    if(EEPROM.read(HOSTNAMEADD) == 0xAA) {
        for (uint8_t iHostName = 1; iHostName <= HOSTNAMESIZE; iHostName++) {
            temp[iHostName - 1] = EEPROM.read(HOSTNAMEADD + iHostName);
        }
        page += temp;
    }
    else {
        page += HOSTNAME;
    }
    page += " configuration page!</h3><br/>";
    page += "<form action=\"/hostnameconfig\" method=\"get\"><button>Configure hostname</button></form><br/>";
    page += "<form action=\"/resetmemory\" method=\"get\"><button>Reset settings</button></form><br/>";
    oWebServer.send(200, "text/html", page);
  });
  oWebServer.on("/hostnameconfig", HTTP_GET, []() {

    // Page css is stored in flash to save RAM
    String page = FPSTR(PAGESTYLE);
    char temp[HOSTNAMESIZE];

    page += "<form action=\"/savehostname\" method=\"post\"> ";
    page += "Setup your system name:<br/> <input type=\"text\" value=\"";

    // Check if there is a hostname already saved in EEPROM
    if(EEPROM.read(HOSTNAMEADD) == 0xAA) {
      for (uint8_t iHostName = 1; iHostName <= HOSTNAMESIZE; iHostName++) {
        temp[iHostName-1] = EEPROM.read(HOSTNAMEADD+iHostName);
      }
      page += temp;
    } else {
      page += HOSTNAME;
    }

    // regexp pattern for hostname control : ^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\-]*[A-Za-z0-9])$ : https://stackoverflow.com/questions/106179/regular-expression-to-match-dns-hostname-or-ip-address
    page += "\" name=\"hostname\" pattern=^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\\-]*[A-Za-z0-9])$ required><br><br> ";
    page += "<button>save hostname</button>";
    page += "</form>";
    oWebServer.send(200, "text/html", page);
  });
  oWebServer.on("/resetmemory", HTTP_GET, []() {
    for (uint16_t iMemoryAddress = 0; iMemoryAddress < 512; iMemoryAddress++)
      EEPROM.write(iMemoryAddress, 0);
    EEPROM.end();

    oWebServer.send(200, "text/html", "<h3>Your system is reseted, please close this window.</h3><script type=\"text/JavaScript\">setTimeout(\"location.href = '/';\",5000);</script>");

    delay(5000);

    WiFi.disconnect(true, true);

    ESP.restart();
  });
  oWebServer.on("/savehostname", HTTP_POST, []() {
    String result = FPSTR(PAGESTYLE);

    // Write Hostname in EEPROM
    if (oWebServer.args()) {
      EEPROM.write(HOSTNAMEADD, 0xAA);
      for (uint8_t iHostName = 1; iHostName <= HOSTNAMESIZE; iHostName++) {
        EEPROM.write(HOSTNAMEADD + iHostName, oWebServer.arg(0)[iHostName - 1]);
      }
      EEPROM.commit();
      oWebServer.send(200, "text/html", "<h3>Your system is now configured, please restart it.</h3><script type=\"text/JavaScript\">setTimeout(\"location.href = '/';\",5000);</script>");
    }
    else {
      oWebServer.send(404, "text/html", "ERROR.");
    }
  });
  oWebServer.on("/update", HTTP_POST, []() {
    oWebServer.sendHeader("Connection", "close");
    oWebServer.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    ESP.restart();
  },
  []() {
    HTTPUpload& upload = oWebServer.upload();
    if(upload.status == UPLOAD_FILE_START){
      update_start();
      SERIAL_DEBUG.printf("Update starting...\n");

      SERIAL_DEBUG.setDebugOutput(true);

      uint32_t maxSketchSpace = 0x1E0000;

      if(!Update.begin(maxSketchSpace)){
        SERIAL_DEBUG.printf("Error on max sketch space : ");
        #if defined(ERROR_ESP_MAIN)
          Update.printError(SERIAL_DEBUG);
        #endif
      }

    } else if(upload.status == UPLOAD_FILE_WRITE){
      update_process(upload.totalSize, upload.totalSize);

      if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
        SERIAL_DEBUG.printf("Error on writing : ");
        #if defined(ERROR_ESP_MAIN)
          Update.printError(SERIAL_DEBUG);
        #endif
      }
    } else if(upload.status == UPLOAD_FILE_END){
      if(Update.end(true)){
        update_end(0);
        SERIAL_DEBUG.printf("Update successful!\n");
      } else {
        update_end(1);
        SERIAL_DEBUG.printf("Error on ending : ");
        #if defined(ERROR_ESP_MAIN)
          Update.printError(SERIAL_DEBUG);
        #endif
      }
      SERIAL_DEBUG.setDebugOutput(false);
    }
    yield();
  });
  oWebServer.begin();

  //Add services to MDNS allowing this module to be easily detected in the network
  MDNS.addService("luos", "tcp", LUOSDETECT);
  MDNS.addService("http", "tcp", SERVERPORT);
  MDNS.addService("luos_bus", "tcp", TCP_PORT);

  // Start listening on TCP port
	moServer = new AsyncServer(TCP_PORT);
	moServer->onClient(&handleNewClient, moServer);
	moServer->begin();

}

/* Clients events */
static void handleError(void* arg, AsyncClient* poClient, int8_t error) {
  SERIAL_DEBUG.printf("TCP Client : handleError\n");
}

// TCP to Serial
static void handleData(void* arg, AsyncClient* poClient, void *data, size_t len) {
	SERIAL_LUOS.write((uint8_t*)data, len);
}

static void handleDisconnect(void* arg, AsyncClient* poClient) {
  SERIAL_DEBUG.printf("TCP Client : handleDisconnect\n");
  connected = 0;
  // Turn off green led
  digitalWrite (GREENLED, LOW);
}

static void handleTimeOut(void* arg, AsyncClient* poClient, uint32_t time) {
  SERIAL_DEBUG.printf("TCP Client : handleTimeOut\n");
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

  SERIAL_DEBUG.printf("TCP Server : New client!\n");
  connected = 1;
  // Turn on green LED
  digitalWrite (GREENLED, HIGH);
}

// Serial to TCP
void loopLuos() {
    static char jsonString[SERIAL_BUFFER_SIZE] = {0};
    static int i = 0;
    int msg_end = 0;
    while(SERIAL_LUOS.available()) {
        jsonString[i] = char(SERIAL_LUOS.read());
        if (jsonString[i] == '\n') {
          msg_end = i;
          i++;
          if (i > SERIAL_BUFFER_SIZE) SERIAL_DEBUG.println("serial buffer overflow ! ");
          break;
        }
        i++;
        if (i > SERIAL_BUFFER_SIZE) SERIAL_DEBUG.println("serial buffer overflow ! ");
    }
    if ((msg_end != 0)) {
        if (connected) {
          // Finish the String just to make sure */
          jsonString[i] = '\0';
          // transfert Json to the current wireless communication way
          moClient->add((char*)jsonString, strlen(jsonString));
          while(!moClient->canSend());
          moClient->send();
        }
        // clean data
        i = 0;
        jsonString[i] = '\0';
    }
}

void setup() {
  // Init SERIAL_DEBUG
  SERIAL_DEBUG.begin(UART_DEBUG_BAUD, SERIAL_DEBUG_PARAM, SERIAL_DEBUG_RXPIN, SERIAL_DEBUG_TXPIN);
  SERIAL_DEBUG.setRxBufferSize(SERIAL_BUFFER_SIZE);
  // Init SERIAL_LUOS
  SERIAL_LUOS.begin(UART_LUOS_BAUD, SERIAL_LUOS_PARAM, SERIAL_LUOS_RXPIN, SERIAL_LUOS_TXPIN);
  SERIAL_LUOS.setRxBufferSize(SERIAL_BUFFER_SIZE);

  SERIAL_DEBUG.printf("Build %s (%s)\n", APP_VERSION, BUILD_TIMESTAMP);
  SERIAL_DEBUG.printf("Serial Buffer Size = %d\n", SERIAL_BUFFER_SIZE);

  // Setup pin LED
  pinMode (GREENLED, OUTPUT);
  pinMode (REDLED, OUTPUT);

  digitalWrite (GREENLED, LOW);
  // Red led blink in hotspot mode and stay on if connected to a wifi
  digitalWrite (REDLED, LOW);

  // Start EEPROM
  EEPROM.begin(512);

  // Retrive hostname
  if(EEPROM.read(HOSTNAMEADD) == 0xAA) {
    for (uint8_t iHostName = 1; iHostName <= HOSTNAMESIZE; iHostName++) {
      hostString[iHostName - 1] = EEPROM.read(HOSTNAMEADD + iHostName);
    }
  } else {
    sprintf(hostString, HOSTNAME);
  }

  SERIAL_DEBUG.printf("Hostname is '%s'\n", hostString);
  WiFi.setHostname(hostString);

  // Configure all communication ways (WiFiManager, WebServerOTA, TCP Server, MDNS)
  CommunicationConfigure();

  // Setup leds
  digitalWrite (REDLED, HIGH);

  // SERIAL_DEBUG.printf("Change WiFi Bandwidth : %X\n", esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20));
  // SERIAL_DEBUG.printf("Change WiFi TX Power  : %X\n", esp_wifi_set_max_tx_power(82));
}

void loop() {
  // SERIAL_DEBUG.printf("HEAP : %8d\n", ESP.getFreeHeap());

  // Blink led for WiFi status
  static unsigned long lastblink = millis();
  if (WiFi.status() != WL_CONNECTED && (millis() - lastblink > 1000)) {
    digitalWrite (REDLED, !digitalRead(REDLED));
    lastblink = millis();
  }

  // If connected to WiFi,
  if (WiFi.status() == WL_CONNECTED) {

    // Handle OTA
    oWebServer.handleClient();

    // Read data on SERIAL_LUOS
    loopLuos();

  }

}
