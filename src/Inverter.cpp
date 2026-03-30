#include "Inverter.h"
#include "Sensor.h"
#include "secrets.h"
#include "Log.h"
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string>

namespace AJ = ArduinoJson;

namespace {
 
#define INSTANCE "deye1"

constexpr const char *const TOPIC_SENSOR_STATE = "homeassistant/sensor/" INSTANCE "/state";

Sensor::HandlerT u_word_handler = [](const Sensor &s, const uint16_t *data, AJ::JsonDocument &doc) {
    doc[s.id] = *data;
};

Sensor::HandlerT u_word_handler_0_1 = [](const Sensor &s, const uint16_t *data, AJ::JsonDocument &doc) {
    doc[s.id] = 0.1 * (*data);
};

Sensor::HandlerT u_word_handler_0_01 = [](const Sensor &s, const uint16_t *data, AJ::JsonDocument &doc) {
    doc[s.id] = 0.01 * (*data);
};

Sensor::HandlerT s_word_handler = [](const Sensor &s, const uint16_t *data, AJ::JsonDocument &doc) {
    doc[s.id] = static_cast<int16_t>(*data);
};

Sensor::HandlerT s_word_handler_0_01 = [](const Sensor &s, const uint16_t *data, AJ::JsonDocument &doc) {
    doc[s.id] = 0.01 * static_cast<int16_t>(*data);
};

const Sensor SENSORS[] = {
    { 70, "battery_daily_charge", "Battery Daily Charge", Unit::kWh, DeviceClass::Energy, StateClass::TotalIncreasing, 1, "mdi:battery-charging", u_word_handler_0_1 },
    { 71, "battery_daily_discharge", "Battery Daily Discharge", Unit::kWh, DeviceClass::Energy, StateClass::TotalIncreasing, 1, "mdi:battery-minus", u_word_handler_0_1 },
    { 76, "grid_daily_bought", "Daily Energy Bought", Unit::kWh, DeviceClass::Energy, StateClass::TotalIncreasing, 1, "mdi:transmission-tower-import", u_word_handler_0_1 },
    { 77, "grid_daily_sold", "Daily Energy Sold", Unit::kWh, DeviceClass::Energy, StateClass::TotalIncreasing, 1, "mdi:transmission-tower-export", u_word_handler_0_1 },
    { 108, "pv_daily", "PV Daily Production", Unit::kWh, DeviceClass::Energy, StateClass::TotalIncreasing, 1, "mdi:solar-power", u_word_handler_0_1 },
    { 109, "pv1_voltage", "PV1 Voltage", Unit::V, DeviceClass::Voltage, StateClass::Measurement, 1, "mdi:solar-power-variant", u_word_handler_0_1 },
    { 110, "pv1_current", "PV1 Current", Unit::A, DeviceClass::Current, StateClass::Measurement, 1, "mdi:solar-power-variant", u_word_handler_0_1 },
    { 111, "pv2_voltage", "PV2 Voltage", Unit::V, DeviceClass::Voltage, StateClass::Measurement, 1, "mdi:solar-power-variant", u_word_handler_0_1 },
    { 112, "pv2_current", "PV2 Current", Unit::A, DeviceClass::Current, StateClass::Measurement, 1, "mdi:solar-power-variant", u_word_handler_0_1 },
    { 152, "grid_voltage", "Grid Voltage", Unit::V, DeviceClass::Voltage, StateClass::Measurement, 1, "mdi:sine-wave", u_word_handler_0_1 },
    { 154, "out_voltage", "Output Voltage", Unit::V, DeviceClass::Voltage, StateClass::Measurement, 1, "mdi:sine-wave", u_word_handler_0_1 },
    { 164, "out_current", "Output Current", Unit::A, DeviceClass::Current, StateClass::Measurement, 2, "mdi:current-ac", u_word_handler_0_01 },
    { 169, "grid_power", "Grid Power", Unit::W, DeviceClass::Power, StateClass::Measurement, 0, "mdi:transmission-tower", s_word_handler },
    { 173, "out_power", "Output Power", Unit::W, DeviceClass::Power, StateClass::Measurement, 0, "mdi:meter-electric", s_word_handler },
    { 183, "battery_voltage", "Battery Voltage", Unit::V, DeviceClass::Voltage, StateClass::Measurement, 2, "mdi:flash", u_word_handler_0_01 },
    { 184, "battery_soc", "Battery SOC", Unit::Percent, DeviceClass::Battery, StateClass::Measurement, 0, "mdi:battery", u_word_handler },
    { 186, "dc_pv1_power", "PV1 Power", Unit::W, DeviceClass::Power, StateClass::Measurement, 0, "mdi:solar-power-variant", u_word_handler },
    { 187, "dc_pv2_power", "PV2 Power", Unit::W, DeviceClass::Power, StateClass::Measurement, 0, "mdi:solar-power-variant", u_word_handler },
    { 190, "battery_power", "Battery Power", Unit::W, DeviceClass::Power, StateClass::Measurement, 0, "mdi:lightning-bolt", s_word_handler },
    { 191, "battery_current", "Battery Current", Unit::A, DeviceClass::Current, StateClass::Measurement, 2, "mdi:current_dc", s_word_handler_0_01 },
};

void AddDevice(AJ::JsonDocument &doc)
{
    auto device = doc["device"].to<AJ::JsonObject>();
    device["identifiers"][0] = INSTANCE;
    device["name"] = "Inverter";
    device["model"] = "2.4";
    device["manufacturer"] = "Deye";
}

void AnnounceSensor(const Sensor &sensor, PubSubClient &client)
{
    char discovery_topic[64];
    sprintf(discovery_topic, "homeassistant/sensor/" INSTANCE "/%s/config", sensor.id);

    AJ::JsonDocument doc;
    AddDevice(doc);
    doc["name"] = sensor.name;
    doc["state_topic"] = TOPIC_SENSOR_STATE;
    doc["unit_of_measurement"] = ToString(sensor.unit_of_measurement);
    doc["value_template"] = "{{ value_json." + std::string{sensor.id} + " }}";
    doc["device_class"] = ToString(sensor.device_class);
    doc["state_class"] = ToString(sensor.state_class);
    doc["unique_id"] = std::string(INSTANCE "_") + sensor.id;
    doc["icon"] = sensor.icon;
    std::string payload;
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

void Inverter::Reconnect()
{
    // Loop until we're reconnected
    while (!_client.connected()) {
        Serial1.print("Attempting MQTT connection...");
        // Attempt to connect
        if (_client.connect("deye1", Secrets::mqtt_user, Secrets::mqtt_pass)) {
            Serial1.println("connected");
            _client.subscribe("homeassistant/+/" INSTANCE "/+/set");
            // Once connected, publish an announcement...
        } else {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    for (const auto &sensor : SENSORS) {
        AnnounceSensor(sensor, _client);
    }

    //AnnounceSensor("mode", "Mode", "", "{{ value_json.mode }}", "None", _client);
    //AnnounceSensor("ac_voltage", "AC Voltage", "V", "{{ value_json.ac_voltage | round }}", "voltage", _client);
    //Announce("pv_volage", "PV Voltage", "V", "{{ value_json.pv_voltage | round }}", "voltage", _client);
    //AnnounceSensor("bat_charge_current", "Bat Charge Current", "A", "{{ value_json.bat_charge_current | round(1) }}", "current", _client);
    //AnnounceSensor("bat_discharge_current", "Bat Discharge Current", "A", "{{ value_json.bat_discharge_current | round(1) }}", "current", _client);
    //AnnounceSensor("output_power", "Output Power", "W", "{{ value_json.output_power | round }}", "power", _client);
    //AnnounceSensor("output_load", "Output Load", "VA", "{{ value_json.output_load | round }}", "apparent_power", _client);
    //AnnounceCurrentSelect(_client);
    //AnnounceBulkVoltage(_client);
    //AnnounceFloatingVoltage(_client);
    //AnnounceDcCutoffVoltage(_client);
}

void Inverter::QueryRegisters()
{
    Log::Info("QueryRegisters\n");

    for (const auto &sensor : SENSORS) {
        auto result = _node.readHoldingRegisters(sensor.address, 1);
        if (result == _node.ku8MBSuccess) {
            uint16_t data = _node.getResponseBuffer(0);

            {
                using namespace ArduinoJson;
                JsonDocument doc;
                if (sensor.handler)
                    sensor.handler(sensor, &data, doc);

                std::string payload;
                payload.reserve(256);
                serializeJson(doc, payload);

                _client.publish(TOPIC_SENSOR_STATE, payload.c_str());
                Log::Info(payload.c_str());
            }

            //char buf[16];
            //sprintf(buf, "%d", ntohs(data[OFFSET_MAX_CURRENT]));
            //_client.publish(HA_SELECT_STATE(MAX_CURRENT), buf);

            //sprintf(buf, "%.1f", 0.1 * ntohs(data[OFFSET_BULK_VOLTAGE]));
            //_client.publish(HA_NUMBER_STATE(BULK_VOLTAGE), buf);
            //sprintf(buf, "%.1f", 0.1 * ntohs(data[OFFSET_FLOATING_VOLTAGE]));
            //_client.publish(HA_NUMBER_STATE(FLOATING_VOLTAGE), buf);
            //sprintf(buf, "%.1f", 0.1 * ntohs(data[OFFSET_CUTOFF_VOLTAGE]));
            //_client.publish(HA_NUMBER_STATE(DC_CUTOFF_VOLTAGE), buf);
        }
    }
}

void Inverter::_HandleCallback(char* topic, byte* payload, unsigned int length)
{
    //using ParserT = uint16_t (*)(const char *);

    //struct Setting
    //{
    //    const char *topic;
    //    uint16_t reg;
    //    ParserT parser;
    //    const char *msg;
    //};

    //ParserT parseInt = [](const char *s) -> uint16_t {
    //    return atoi(s);
    //};

    //ParserT parseFloat = [](const char *s) -> uint16_t {
    //    return atof(s) * 10;
    //};

    //const Setting SETTINGS[] = {
    //    {HA_SELECT_SET(MAX_CURRENT), 5022, parseInt, "current"},
    //    {HA_NUMBER_SET(BULK_VOLTAGE), 5027, parseFloat, "bulk charging voltage"},
    //    {HA_NUMBER_SET(FLOATING_VOLTAGE), 5028, parseFloat, "floating charge voltage"},
    //    {HA_NUMBER_SET(DC_CUTOFF_VOLTAGE), 5029, parseFloat, "DC cutoff voltage"},
    //};

    //for (const auto &setting : SETTINGS) {
    //    if (!strcmp(topic, setting.topic)) {
    //        payload[length] = '\0';
    //        uint16_t cc = setting.parser(reinterpret_cast<const char *>(payload));
    //        for (int i = 0; i < 5; ++i) {
    //            auto result = _node.writeSingleRegister(setting.reg, cc);
    //            if (result == _node.ku8MBSuccess)
    //                return;
    //            char buf[64];
    //            sprintf(buf, "Failed to change charge current: %x", (unsigned)result);
    //            _client.publish(HA_DEBUG("log"), buf);
    //        }
    //        //_client.publish(publishTopicLog, buf);
    //        return;
    //    }
    //}
}
