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

struct RegHandler
{
    uint8_t offset;
    const char *topic;
    void (*handler)(const char *topic, uint16_t val);
};

void PublishHex(const char *topic, uint16_t val)
{
    auto op_mode = htons(val);
    char str[16];
    snprintf(str, 5, "%04x", op_mode);
    Serial1.printf("%s\t%.*s\n", topic, (int)strlen(str), str);
    client.publish(topic, str, strlen(str)); 
}

void PublishDeciUnit(const char *topic, uint16_t val)
{
    float v = 0.1 * htons(val);
    Serial1.printf("%s\t%f\n", topic, v);
    publish_float(topic, v);
}

void PublishHalfDeciUnit(const char *topic, uint16_t val)
{
    float v = 0.05 * htons(val);
    Serial1.printf("%s\t%f\n", topic, v);
    publish_float(topic, v);
}

void PublishUnit(const char *topic, uint16_t val)
{
    float v = htons(val);
    Serial1.printf("%s\t%f\n", topic, v);
    publish_float(topic, v);
}

constexpr const RegHandler REG_HANDLERS[] = {
    { 0,  "powmr/1/mode",                   PublishHex          },
    { 1,  "powmr/1/ac_voltage",             PublishDeciUnit     },
    { 3,  "powmr/1/pv_voltage",             PublishDeciUnit     },
    { 4,  "powmr/1/pv_power",               PublishUnit         },
    { 5,  "powmr/1/bat_voltage",            PublishDeciUnit     },
    { 7,  "powmr/1/bat_charge_current",     PublishUnit         },
    { 8,  "powmr/1/bat_discharge_current",  PublishUnit         },
    { 11, "powmr/1/output_power",           PublishUnit         },
    { 12, "powmr/1/output_load",            PublishHalfDeciUnit },
};

void query_registers() {
    constexpr const int REGNUM = REG_HANDLERS[std::size(REG_HANDLERS) - 1].offset + 1;

    Serial1.println("query_registers");
    auto result = node.readHoldingRegisters(4501, REGNUM);
    if (result == node.ku8MBSuccess)
    {
        uint16_t data[REGNUM];
        for (uint8_t i = 0; i < REGNUM; ++i) {
            data[i] = node.getResponseBuffer(i);
        }

        for (auto h : REG_HANDLERS) {
            h.handler(h.topic, data[h.offset]);
        }
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
