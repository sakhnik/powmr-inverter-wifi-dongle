#include "Inverter.h"
#include "secrets.h"
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>

namespace {
 
constexpr const char *const TOPIC = "homeassistant/sensor/powmr1/state";

void Announce(const char *sensor, const char *name, const char *unit, const char *value_template, const char *device_class, PubSubClient &client)
{
    char discovery_topic[64];
    sprintf(discovery_topic, "homeassistant/sensor/powmr1/%s/config", sensor);

    String payload;
    payload.reserve(1024);
    payload = "{\"device\":{\"identifiers\":[\"powmr1\"],\"name\":\"Inverter\",\"model\":\"2.4\",\"manufacturer\":\"PowMr\"}";
    payload += ",\"name\":\"";
    payload += name;
    payload += "\",\"state_topic\":\"";
    payload += TOPIC;
    payload += "\",\"unit_of_measurement\":\"";
    payload += unit;
    payload += "\",\"value_template\":\"";
    payload += value_template;
    payload += "\",\"device_class\":\"";
    payload += device_class;
    payload += "\",\"unique_id\":\"";
    payload += "powmr1_";
    payload += sensor;
    payload += "\"}";
    bool res = client.publish(discovery_topic, payload.c_str(), true);
    if (!res) {
        Serial1.print("Announce failure: ");
        Serial1.print(discovery_topic);
        Serial1.print(" ");
        Serial1.println(res);
    }
}

} //namespace;

Inverter::Inverter(WiFiClient &wifi, ModbusMaster &node)
    : _wifi{wifi}
    , _node{node}
    , _client(_wifi)
{
    // Allocate enough space to fit announcement payload
    _client.setBufferSize(1024);
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

    Announce("mode", "Mode", "", "{{ value_json.mode }}", "None", _client);
    Announce("ac_voltage", "AC Voltage", "V", "{{ value_json.ac_voltage | round }}", "voltage", _client);
    Announce("pv_volage", "PV Voltage", "V", "{{ value_json.pv_voltage | round }}", "voltage", _client);
    Announce("pv_power", "PV Power", "W", "{{ value_json.pv_power | round }}", "power", _client);
    Announce("bat_voltage", "Bat Voltage", "V", "{{ value_json.bat_voltage | round }}", "voltage", _client);
    Announce("bat_charge_current", "Bat Charge Current", "A", "{{ value_json.bat_charge_current | round(1) }}", "current", _client);
    Announce("bat_discharge_current", "Bat Discharge Current", "A", "{{ value_json.bat_discharge_current | round(1) }}", "current", _client);
    Announce("output_power", "Output Power", "W", "{{ value_json.output_power | round }}", "power", _client);
    Announce("output_load", "Output Load", "VA", "{{ value_json.output_load | round }}", "apparent_power", _client);
}

namespace {

void HandleFloat(const char *name, float var, String &payload)
{
    char buf[16];
    sprintf(buf, "%3.1f", var);
    payload += "\"";
    payload += name;
    payload += "\":";
    payload += buf;
}

struct RegHandler
{
    uint8_t offset;
    const char *name;
    void (*handler)(const char *name, uint16_t val, String &);
};

void HandleHex(const char *name, uint16_t val, String &payload)
{
    auto op_mode = htons(val);
    char str[16];
    sprintf(str, "%04x", op_mode);
    payload += "\"";
    payload += name;
    payload += "\":\"";
    payload += str;
    payload += "\"";
}

void HandleDeciUnit(const char *name, uint16_t val, String &payload)
{
    float v = 0.1 * htons(val);
    HandleFloat(name, v, payload);
}

void HandleHalfDeciUnit(const char *name, uint16_t val, String &payload)
{
    float v = 0.05 * htons(val);
    HandleFloat(name, v, payload);
}

void HandleUnit(const char *name, uint16_t val, String &payload)
{
    float v = htons(val);
    HandleFloat(name, v, payload);
}

constexpr const RegHandler REG_HANDLERS[] = {
    { 0,  "mode",                   HandleHex          },
    { 1,  "ac_voltage",             HandleDeciUnit     },
    { 3,  "pv_voltage",             HandleDeciUnit     },
    { 4,  "pv_power",               HandleUnit         },
    { 5,  "bat_voltage",            HandleDeciUnit     },
    { 7,  "bat_charge_current",     HandleUnit         },
    { 8,  "bat_discharge_current",  HandleUnit         },
    { 11, "output_power",           HandleUnit         },
    { 12, "output_load",            HandleHalfDeciUnit },
};

} //namespace;

void Inverter::QueryRegisters()
{
    constexpr const int REGNUM = REG_HANDLERS[std::size(REG_HANDLERS) - 1].offset + 1;

    auto result = _node.readHoldingRegisters(4501, REGNUM);
    if (result == _node.ku8MBSuccess)
    {
        uint16_t data[REGNUM];
        for (uint8_t i = 0; i < REGNUM; ++i) {
            data[i] = _node.getResponseBuffer(i);
        }

        String payload;
        payload.reserve(256);
        payload = "{";

        for (const auto &h : REG_HANDLERS) {
            if (&h != REG_HANDLERS) {
                payload += ",";
            }
            h.handler(h.name, data[h.offset], payload);
        }

        payload += "}";
        _client.publish(TOPIC, payload.c_str());
    }
}
