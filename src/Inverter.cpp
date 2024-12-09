#include "Inverter.h"
#include "secrets.h"
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>

Inverter::Inverter(WiFiClient &wifi, ModbusMaster &node)
    : _wifi{wifi}
    , _node{node}
    , _client(_wifi)
{
}

void Inverter::Setup()
{
    _client.setServer(Secrets::mqtt_server, 1883);
    _task = _timer.every(10000, [](void *ctx) {
        Inverter *self = reinterpret_cast<Inverter *>(ctx);
        self->QueryRegisters();
        return true;
    }, this);
}

void Inverter::Handle()
{
    if (!_client.connected()) {
        Reconnect();
    }
    _client.loop();
    _timer.tick();
}

void Inverter::Reconnect() {
    // Loop until we're reconnected
    while (!_client.connected()) {
        Serial1.print("Attempting MQTT connection...");
        // Attempt to connect
        if (_client.connect("powmr", Secrets::mqtt_user, Secrets::mqtt_pass)) {
            Serial1.println("connected");
            // Once connected, publish an announcement...
        } else {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

namespace {

void publish_float(const char *topic, float var, PubSubClient &client)
{
    char buf[16];
    sprintf(buf, "%3.1f", var);
    client.publish(topic, buf);
}

struct RegHandler
{
    uint8_t offset;
    const char *topic;
    void (*handler)(const char *topic, uint16_t val, PubSubClient &);
};

template <typename T>
void Print(const char *topic, T val)
{
    Serial1.print(topic);
    Serial1.print("\t");
    Serial1.println(val);
}

void PublishHex(const char *topic, uint16_t val, PubSubClient &client)
{
    auto op_mode = htons(val);
    char str[16];
    sprintf(str, "%04x", op_mode);
    Print(topic, str);
    client.publish(topic, str); 
}

void PublishDeciUnit(const char *topic, uint16_t val, PubSubClient &client)
{
    float v = 0.1 * htons(val);
    Print(topic, v);
    publish_float(topic, v, client);
}

void PublishHalfDeciUnit(const char *topic, uint16_t val, PubSubClient &client)
{
    float v = 0.05 * htons(val);
    Print(topic, v);
    publish_float(topic, v, client);
}

void PublishUnit(const char *topic, uint16_t val, PubSubClient &client)
{
    float v = htons(val);
    Print(topic, v);
    publish_float(topic, v, client);
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

} //namespace;

void Inverter::QueryRegisters()
{
    constexpr const int REGNUM = REG_HANDLERS[std::size(REG_HANDLERS) - 1].offset + 1;

    Serial1.println("query_registers");
    auto result = _node.readHoldingRegisters(4501, REGNUM);
    if (result == _node.ku8MBSuccess)
    {
        uint16_t data[REGNUM];
        for (uint8_t i = 0; i < REGNUM; ++i) {
            data[i] = _node.getResponseBuffer(i);
        }

        for (auto h : REG_HANDLERS) {
            h.handler(h.topic, data[h.offset], _client);
        }
    }
}
