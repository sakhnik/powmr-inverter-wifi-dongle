//#include "config.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SimpleTimer.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>


#define OTA_HOSTNAME                    "powmr1"
#define POWMR_DEVICE_ID    5
#define MQTT_ROOT                       "powmr1"

WiFiClient espClient;
PubSubClient client(espClient);

ModbusMaster node;
SimpleTimer timer(10000);

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

    client.setServer(Secrets::mqtt_server, 1883);

    timer.setInterval(10000);
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial1.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("powmr", Secrets::mqtt_user, Secrets::mqtt_pass)) {
            Serial1.println("connected");
            // Once connected, publish an announcement...
        } else {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void publish_float(const char *topic, float var)
{
    char buf[11];
    snprintf(buf, 10,"%3.1f", var);
    client.publish(topic, buf);
}

void query_registers() {
    auto result = node.readHoldingRegisters(4501, 45);
    if (result == node.ku8MBSuccess)
    {
        //ctemp = (long)node.getResponseBuffer(0x11) / 100.0f; 
        //dtostrf(ctemp, 2, 3, buf );
        ////mqtt_location = MQTT_ROOT + "/" + POWMR_DEVICE_ID + "/ctemp";
        //client.publish("powmr/1/ctemp", buf);

        float bvoltage = htons(node.getResponseBuffer(0x5)) / 10.0;
        Serial1.print("bvoltage=");
        Serial1.println(bvoltage);
        publish_float("powmr/1/bvoltage", bvoltage);

    }
}

void loop() {
    ArduinoOTA.handle();
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    if (timer.isReady()) {
        query_registers();
        timer.reset();
    }
}
