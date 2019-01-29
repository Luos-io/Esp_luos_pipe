#include "config.h"
#include "webpage.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#include <EEPROM.h>

WebServer server(SERVERPORT);

// Is this an IP?
boolean isIp(String str) {
    for (int i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

// IP to String conversion
String toStringIp(IPAddress ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}


// Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again.
boolean captivePortal() {
    if (!isIp(server.hostHeader()) ) {
        Serial.println("Request redirected to captive portal");
        server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
        server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
        server.client().stop(); // Stop is needed because we sent no content length
        return true;
    }
    return false;
}

// Page used to setup hostname
void hostnameconfig(void) {
    // Page css is stored in flash to save RAM
    String page = FPSTR(PAGESTYLE);
    char temp[HOSTNAMESIZE];

    page += "<form action=\"/savehostname\" method=\"post\"> ";
    page += "Setup your system name:<br/> <input type=\"text\" value=\"";
    // Check if there is a hostname already saved in EEPROM
    if(EEPROM.read(HOSTNAMEADD) == 0xAA) {
        for (int i=1; i<=HOSTNAMESIZE; i++) {
            temp[i-1] = EEPROM.read(HOSTNAMEADD+i);
        }
        page += temp;
    }
    else {
        page += HOSTNAME;
    }
    // regexp pattern for hostname control : ^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\-]*[A-Za-z0-9])$ : https://stackoverflow.com/questions/106179/regular-expression-to-match-dns-hostname-or-ip-address
    page += "\" name=\"hostname\" pattern=^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\\-]*[A-Za-z0-9])$ required><br><br> ";
    page += "<button>save hostname</button>";
    page += "</form>";
    server.send(200, "text/html", page);
}

// WiFi.scanNetworks will return the number of networks found
String webWifiScan(void) {
    String result = "";
    char temp [100];
    int n = WiFi.scanNetworks();
    if (n == 0) {
        result += "no networks found\n";
    } else {
        sprintf(temp, "<h3>%d networks found</h3><br/>", n);
        result += temp;
        result += "<form action=\"/savessid\" method=\"post\">";
        result += "<div style=\"text-align:left;  margin-left: 28%; margin-right: 20%;\">";
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            result += "<input type=\"radio\" name=\"SSID\" value=\"";
            result += WiFi.SSID(i);
            result += "\">";
            sprintf(temp, "%d: ", i + 1);
            result += temp;
            result += WiFi.SSID(i);
            sprintf(temp, " (%d)", WiFi.RSSI(i));
            result += temp;
            result += ((WiFi.encryptionType(i) == 7)?"":" &#128274;");
            result += "<br>";
            delay(10);
        }
        result += "</div><button>Save wifi config</button>";
        result += "</form>";
        return result;
    }
    return result;
}

// Erase the EEPROM to get back default values
void resetmemory(void) {
    for (int i = 0; i < 512; i++)
        EEPROM.write(i, 0);
    EEPROM.end();
    //Write a small message and get back after 5s (HTML script)
    server.send(200, "text/html", "<h3>Your system is reseted, please restart it.</h3><script type=\"text/JavaScript\">setTimeout(\"location.href = '/';\",5000);</script>");
}

void savessid(void) {
    // Page css is stored in flash to save RAM
    String result = FPSTR(PAGESTYLE);
    // write previously selected SSID (savehostname()) in EEPROM
    if (server.args()) {
        for (int i=0; i<SSIDSIZE; i++) {
            EEPROM.write(SSIDADD+i, server.arg(0)[i]);
        }
        EEPROM.commit();
        // Create a password form
        result += "<form action=\"/savepass\" method=\"post\">";
        result += "<input type=\"password\" name=\"password\" size=\"39\"><br>";
        result += "<button>Save password</button>";
        server.send(200, "text/html", result);
    }
    else {
        server.send(404, "text/html", "ERROR.");
    }
}

// Save password
void savepass(void) {
    // Write previously selected (savessid()) password in EEPROM
    if (server.args()) {
        for (int i=0; i<PASSSIZE; i++) {
            EEPROM.write(PASSADD+i, server.arg(0)[i]);
        }
        EEPROM.commit();
        //Write a small message and get back after 5s (HTML script)
        server.send(200, "text/html", "<h3>Your system is now configured, please restart it.</h3><script type=\"text/JavaScript\">setTimeout(\"location.href = '/';\",5000);</script>");
    }
    else {
        server.send(404, "text/html", "ERROR.");
    }
}

// Save hostname and print scanned SSID
void savehostname(void) {
    String result = FPSTR(PAGESTYLE);
    // Write Hostname in EEPROM
    if (server.args()) {
        EEPROM.write(HOSTNAMEADD, 0xAA);
        for (int i=1; i<=HOSTNAMESIZE; i++) {
            EEPROM.write(HOSTNAMEADD+i, server.arg(0)[i-1]);
        }
        EEPROM.commit();
        // Scan wifi and create HTML with it
        result += webWifiScan();
        server.send(200, "text/html", result);
    }
    else {
        server.send(404, "text/html", "ERROR.");
    }
}

void handleRoot(void) {
    // If caprive portal, redirect to IP website instead of displaying the page.
    if (captivePortal()) {
      return;
    }

    // Page css is stored in flash to save RAM
    String page = FPSTR(PAGESTYLE);
    page += "<h3>Welcome to the ";
    char temp[HOSTNAMESIZE];
    // Check if there is an hostname saved and put it into HTML
    if(EEPROM.read(HOSTNAMEADD) == 0xAA) {
        for (int i=1; i<=HOSTNAMESIZE; i++) {
            temp[i-1] = EEPROM.read(HOSTNAMEADD+i);
        }
        page += temp;
    }
    else {
        page += HOSTNAME;
    }
    page += " configuration page!</h3><br/>";
    page += "<form action=\"/hostnameconfig\" method=\"get\"><button>Configure WiFi</button></form><br/>";
    page += "<form action=\"/resetmemory\" method=\"get\"><button>Reset settings</button></form><br/>";
    server.send(200, "text/html", page);
}

// Manage page not found
void handleNotFound(void){
    // If caprive portal, redirect to IP website instead of displaying the page.
    if (captivePortal()) {
        return;
    }
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

// Server init and setup page callbacks
void pageinit(){
    // handle index
    server.on("/", handleRoot);
    server.on("/hostnameconfig", hostnameconfig);
    server.on("/savehostname", savehostname);
    server.on("/savessid", savessid);
    server.on("/savepass", savepass);
    server.on("/resetmemory", resetmemory);
    server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.

    server.onNotFound(handleNotFound);

    server.begin();
}

void pageloop(void){
    server.handleClient();
}
