//#include "config.h"
#include "secrets.h"
#include "Inverter.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ModbusMaster.h>


constexpr const char *const OTA_HOSTNAME = "powmr1";
constexpr const auto POWMR_DEVICE_ID = 5;

WiFiClient espClient;
ModbusMaster node;
Inverter inverter{espClient, node};

void setup() {
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);

    Serial.begin(2400);
    Serial1.begin(9600);
    Serial1.println("Booting");
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(Secrets::ssid, Secrets::password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial1.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    Serial1.print("Wifi connected, IP address: ");
    Serial1.println(WiFi.localIP());

    // Modbus slave ID 1
    node.begin(POWMR_DEVICE_ID, Serial);
    node.preTransmission([]() {
        digitalWrite(4, HIGH);
        digitalWrite(5, HIGH);
    });
    node.postTransmission([]() {
        digitalWrite(4, LOW);
        digitalWrite(5, LOW);
    });

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.begin();

    inverter.Setup();
}

void loop() {
    ArduinoOTA.handle();
    inverter.Handle();
}
