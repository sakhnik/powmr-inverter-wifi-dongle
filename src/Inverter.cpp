#include "Inverter.h"
#include "secrets.h"
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.hpp>

namespace AJ = ArduinoJson;

namespace {
 
#define POWMR "powmr1"

#define HA_SELECT(name)         "homeassistant/select/" POWMR "/" name
#define HA_SELECT_CONFIG(name)  HA_SELECT(name) "/config"
#define HA_SELECT_SET(name)     HA_SELECT(name) "/set"
#define HA_SELECT_STATE(name)   HA_SELECT(name) "/state"

#define MAX_CURRENT             "total_charging_current"

#define HA_NUMBER(name)         "homeassistant/number/" POWMR "/" name
#define HA_NUMBER_CONFIG(name)  HA_NUMBER(name) "/config"
#define HA_NUMBER_SET(name)     HA_NUMBER(name) "/set"
#define HA_NUMBER_STATE(name)   HA_NUMBER(name) "/state"

#define BULK_VOLTAGE            "bulk_charging_voltage"
#define FLOATING_VOLTAGE        "floating_charging_voltage"
#define DC_CUTOFF_VOLTAGE       "low_dc_cutoff_voltage"

#define HA_DEBUG(name)          "homeassistant/debug/" POWMR "/" name

constexpr const char *const TOPIC_SENSOR_STATE = "homeassistant/sensor/" POWMR "/state";

void AddDevice(AJ::JsonDocument &doc)
{
    auto device = doc[F("device")].to<AJ::JsonObject>();
    device["identifiers"][0] = POWMR;
    device["name"] = "Inverter";
    device["model"] = "2.4";
    device["manufacturer"] = "PowMr";
}

void AnnounceSensor(const char *sensor, const char *name, const char *unit, const char *value_template, const char *device_class, PubSubClient &client)
{
    char discovery_topic[64];
    sprintf(discovery_topic, "homeassistant/sensor/" POWMR "/%s/config", sensor);

    AJ::JsonDocument doc;
    AddDevice(doc);
    doc["name"] = name;
    doc["state_topic"] = TOPIC_SENSOR_STATE;
    doc["unit_of_measurement"] = unit;
    doc["value_template"] = value_template;
    doc["device_class"] = device_class;
    doc["unique_id"] = String("powmr1_") + sensor;
    String payload;
    payload.reserve(1024);
    serializeJson(doc, payload);

    bool res = client.publish(discovery_topic, payload.c_str(), true);
    if (!res) {
        Serial1.print("Announce failure: ");
        Serial1.print(discovery_topic);
        Serial1.print(" ");
        Serial1.println(res);
    }
}

void AnnounceCurrentSelect(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);
    doc[F("name")] = F("Total charging current");
    doc[F("state_topic")] = F(HA_SELECT_STATE(MAX_CURRENT));
    doc[F("command_topic")] = F(HA_SELECT_SET(MAX_CURRENT));
    auto options = doc[F("options")].to<AJ::JsonArray>();
    options[0] = F("0");
    options[1] = F("10");
    options[2] = F("20");
    options[3] = F("30");
    doc[F("unique_id")] = F("powmr1_" MAX_CURRENT);
    String payload;
    payload.reserve(1024);
    serializeJson(doc, payload);

    const char *discovery_topic = HA_SELECT_CONFIG(MAX_CURRENT);
    bool res = client.publish(discovery_topic, payload.c_str(), true);
    if (!res) {
        Serial1.print("Announce failure: ");
        Serial1.print(discovery_topic);
        Serial1.print(" ");
        Serial1.println(res);
    }
}

void AnnounceBulkVoltage(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);
    doc[F("name")] = F("Bulk charging voltage");
    doc[F("state_topic")] = F(HA_NUMBER_STATE(BULK_VOLTAGE));
    doc[F("command_topic")] = F(HA_NUMBER_SET(BULK_VOLTAGE));
    doc[F("unique_id")] = F("powmr1_" BULK_VOLTAGE);
    doc[F("min")] = 26.8;
    doc[F("max")] = 29.2;
    doc[F("step")] = 0.1;
    doc[F("unit_of_measurement")] = "V";
    String payload;
    payload.reserve(1024);
    serializeJson(doc, payload);

    const char *discovery_topic = HA_NUMBER_CONFIG(BULK_VOLTAGE);
    bool res = client.publish(discovery_topic, payload.c_str(), true);
    if (!res) {
        Serial1.print("Announce failure: ");
        Serial1.print(discovery_topic);
        Serial1.print(" ");
        Serial1.println(res);
    }
}

void AnnounceFloatingVoltage(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);
    doc[F("name")] = F("Floating charging voltage");
    doc[F("state_topic")] = F(HA_NUMBER_STATE(FLOATING_VOLTAGE));
    doc[F("command_topic")] = F(HA_NUMBER_SET(FLOATING_VOLTAGE));
    doc[F("unique_id")] = F("powmr1_" FLOATING_VOLTAGE);
    doc[F("min")] = 26.6;
    doc[F("max")] = 28.0;
    doc[F("step")] = 0.1;
    doc[F("unit_of_measurement")] = "V";
    String payload;
    payload.reserve(1024);
    serializeJson(doc, payload);

    const char *discovery_topic = HA_NUMBER_CONFIG(FLOATING_VOLTAGE);
    bool res = client.publish(discovery_topic, payload.c_str(), true);
    if (!res) {
        Serial1.print("Announce failure: ");
        Serial1.print(discovery_topic);
        Serial1.print(" ");
        Serial1.println(res);
    }
}

void AnnounceDcCutoffVoltage(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);
    doc[F("name")] = F("Low DC cut-off voltage");
    doc[F("state_topic")] = F(HA_NUMBER_STATE(DC_CUTOFF_VOLTAGE));
    doc[F("command_topic")] = F(HA_NUMBER_SET(DC_CUTOFF_VOLTAGE));
    doc[F("unique_id")] = F("powmr1_" DC_CUTOFF_VOLTAGE);
    doc[F("min")] = 20.0;
    doc[F("max")] = 24.0;
    doc[F("step")] = 0.1;
    doc[F("unit_of_measurement")] = "V";
    String payload;
    payload.reserve(1024);
    serializeJson(doc, payload);

    const char *discovery_topic = HA_NUMBER_CONFIG(DC_CUTOFF_VOLTAGE);
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
    _client.setCallback([this](char *topic, byte *payload, unsigned int length) { _HandleCallback(topic, payload, length); });
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
            _client.subscribe("homeassistant/+/" POWMR "/+/set");
            // Once connected, publish an announcement...
        } else {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    AnnounceSensor("mode", "Mode", "", "{{ value_json.mode }}", "None", _client);
    AnnounceSensor("ac_voltage", "AC Voltage", "V", "{{ value_json.ac_voltage | round }}", "voltage", _client);
    //Announce("pv_volage", "PV Voltage", "V", "{{ value_json.pv_voltage | round }}", "voltage", _client);
    //Announce("pv_power", "PV Power", "W", "{{ value_json.pv_power | round }}", "power", _client);
    AnnounceSensor("bat_voltage", "Bat Voltage", "V", "{{ value_json.bat_voltage | round(1) }}", "voltage", _client);
    AnnounceSensor("bat_charge_current", "Bat Charge Current", "A", "{{ value_json.bat_charge_current | round(1) }}", "current", _client);
    AnnounceSensor("bat_discharge_current", "Bat Discharge Current", "A", "{{ value_json.bat_discharge_current | round(1) }}", "current", _client);
    AnnounceSensor("output_power", "Output Power", "W", "{{ value_json.output_power | round }}", "power", _client);
    AnnounceSensor("output_load", "Output Load", "VA", "{{ value_json.output_load | round }}", "apparent_power", _client);
    AnnounceCurrentSelect(_client);
    AnnounceBulkVoltage(_client);
    AnnounceFloatingVoltage(_client);
    AnnounceDcCutoffVoltage(_client);
}

namespace {

void HandleFloat(const char *name, float var, AJ::JsonDocument &doc)
{
    doc[name] = var;
}

struct RegHandler
{
    uint8_t offset;
    const char *name;
    void (*handler)(const char *name, uint16_t val, AJ::JsonDocument &);
};

void HandleHex(const char *name, uint16_t val, AJ::JsonDocument &doc)
{
    auto op_mode = ntohs(val);
    char str[16];
    sprintf(str, "%04x", op_mode);
    doc[name] = str;
}

void HandleDeciUnit(const char *name, uint16_t val, AJ::JsonDocument &doc)
{
    float v = 0.1 * ntohs(val);
    HandleFloat(name, v, doc);
}

void HandleHalfDeciUnit(const char *name, uint16_t val, AJ::JsonDocument &doc)
{
    float v = 0.05 * ntohs(val);
    HandleFloat(name, v, doc);
}

void HandleUnit(const char *name, uint16_t val, AJ::JsonDocument &doc)
{
    float v = ntohs(val);
    HandleFloat(name, v, doc);
}

constexpr const int OFFSET_MAX_CURRENT = 40;
constexpr const int OFFSET_BULK_VOLTAGE = 45;
constexpr const int OFFSET_FLOATING_VOLTAGE = 46;
constexpr const int OFFSET_CUTOFF_VOLTAGE = 47;

constexpr const RegHandler REG_HANDLERS[] = {
    { 0,  "mode",                   HandleHex           },
    { 1,  "ac_voltage",             HandleDeciUnit      },
    //{ 3,  "pv_voltage",             HandleDeciUnit     },
    //{ 4,  "pv_power",               HandleUnit         },
    { 5,  "bat_voltage",            HandleDeciUnit      },
    { 7,  "bat_charge_current",     HandleUnit          },
    { 8,  "bat_discharge_current",  HandleUnit          },
    { 11, "output_power",           HandleUnit          },
    { 12, "output_load",            HandleHalfDeciUnit  },
    { OFFSET_MAX_CURRENT, nullptr,  nullptr             },
    { OFFSET_BULK_VOLTAGE, nullptr, nullptr             },
    { OFFSET_FLOATING_VOLTAGE, nullptr, nullptr         },
    { OFFSET_CUTOFF_VOLTAGE, nullptr, nullptr           },
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

        {
            using namespace ArduinoJson;
            JsonDocument doc;
            for (const auto &h : REG_HANDLERS) {
                if (h.handler)
                    h.handler(h.name, data[h.offset], doc);
            }

            String payload;
            payload.reserve(256);
            serializeJson(doc, payload);

            _client.publish(TOPIC_SENSOR_STATE, payload.c_str());
        }

        char buf[16];
        sprintf(buf, "%d", ntohs(data[OFFSET_MAX_CURRENT]));
        _client.publish(HA_SELECT_STATE(MAX_CURRENT), buf);

        sprintf(buf, "%.1f", 0.1 * ntohs(data[OFFSET_BULK_VOLTAGE]));
        _client.publish(HA_NUMBER_STATE(BULK_VOLTAGE), buf);
        sprintf(buf, "%.1f", 0.1 * ntohs(data[OFFSET_FLOATING_VOLTAGE]));
        _client.publish(HA_NUMBER_STATE(FLOATING_VOLTAGE), buf);
        sprintf(buf, "%.1f", 0.1 * ntohs(data[OFFSET_CUTOFF_VOLTAGE]));
        _client.publish(HA_NUMBER_STATE(DC_CUTOFF_VOLTAGE), buf);
    }
}

void Inverter::_HandleCallback(char* topic, byte* payload, unsigned int length)
{
    using ParserT = uint16_t (*)(const char *);

    struct Setting
    {
        const char *topic;
        uint16_t reg;
        ParserT parser;
        const char *msg;
    };

    ParserT parseInt = [](const char *s) -> uint16_t {
        return atoi(s);
    };

    ParserT parseFloat = [](const char *s) -> uint16_t {
        return atof(s) * 10;
    };

    const Setting SETTINGS[] = {
        {HA_SELECT_SET(MAX_CURRENT), 5022, parseInt, "current"},
        {HA_NUMBER_SET(BULK_VOLTAGE), 5027, parseFloat, "bulk charging voltage"},
        {HA_NUMBER_SET(FLOATING_VOLTAGE), 5028, parseFloat, "floating charge voltage"},
        {HA_NUMBER_SET(DC_CUTOFF_VOLTAGE), 5029, parseFloat, "DC cutoff voltage"},
    };

    for (const auto &setting : SETTINGS) {
        if (!strcmp(topic, setting.topic)) {
            payload[length] = '\0';
            uint16_t cc = setting.parser(reinterpret_cast<const char *>(payload));
            for (int i = 0; i < 5; ++i) {
                auto result = _node.writeSingleRegister(setting.reg, cc);
                if (result == _node.ku8MBSuccess)
                    return;
                char buf[64];
                sprintf(buf, "Failed to change charge current: %x", (unsigned)result);
                _client.publish(HA_DEBUG("log"), buf);
            }
            //_client.publish(publishTopicLog, buf);
            return;
        }
    }
}
