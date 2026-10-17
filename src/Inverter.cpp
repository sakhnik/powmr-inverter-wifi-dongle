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

constexpr const char *const TOPIC_SENSOR_STATE = INSTANCE "/state";
constexpr const char *const TOPIC_TOU_STATE = INSTANCE "/tou/state";
constexpr const char *const TOPIC_TOU_SET = INSTANCE "/tou/set";
constexpr const char *const TOPIC_MAX_CHARGING_CURRENT_STATE = INSTANCE "/max_charging_current/state";
constexpr const char *const TOPIC_MAX_CHARGING_CURRENT_SET = INSTANCE "/max_charging_current/set";
constexpr const char *const TOPIC_GRID_CHARGING_CURRENT_STATE = INSTANCE "/grid_charging_current/state";
constexpr const char *const TOPIC_GRID_CHARGING_CURRENT_SET = INSTANCE "/grid_charging_current/set";

Sensor::HandlerT u_word_handler = [](const Sensor &s, uint16_t data, AJ::JsonDocument &doc) {
    doc[s.id] = data;
};

Sensor::HandlerT u_word_handler_0_1 = [](const Sensor &s, uint16_t data, AJ::JsonDocument &doc) {
    doc[s.id] = 0.1 * data;
};

Sensor::HandlerT u_word_handler_0_01 = [](const Sensor &s, uint16_t data, AJ::JsonDocument &doc) {
    doc[s.id] = 0.01 * data;
};

Sensor::HandlerT s_word_handler = [](const Sensor &s, uint16_t data, AJ::JsonDocument &doc) {
    doc[s.id] = static_cast<int16_t>(data);
};

Sensor::HandlerT s_word_handler_0_01 = [](const Sensor &s, uint16_t data, AJ::JsonDocument &doc) {
    doc[s.id] = 0.01 * static_cast<int16_t>(data);
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

void AnnounceTou(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);

    doc["name"] = "TOU Schedule";
    doc["unique_id"] = INSTANCE "_tou";
    doc["state_topic"] = TOPIC_TOU_STATE;
    doc["command_topic"] = TOPIC_TOU_SET;
    doc["icon"] = "mdi:calendar-clock";

    std::string payload;
    serializeJson(doc, payload);

    client.publish("homeassistant/text/" INSTANCE "/tou/config", payload.c_str(), true);
}

void AnnounceMaxChargingCurrent(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);

    doc["name"] = "Max Charging Current";
    doc["unique_id"] = INSTANCE "_max_charging_current";

    doc["state_topic"] = TOPIC_MAX_CHARGING_CURRENT_STATE;
    doc["command_topic"] = TOPIC_MAX_CHARGING_CURRENT_SET;

    doc["min"] = 0;
    doc["max"] = 100;
    doc["step"] = 1;
    doc["unit_of_measurement"] = "A";
    doc["device_class"] = "current";
    doc["entity_category"] = "config";
    doc["mode"] = "slider";
    doc["icon"] = "mdi:battery-charging";

    std::string payload;
    serializeJson(doc, payload);

    client.publish("homeassistant/number/" INSTANCE "/max_charging_current/config", payload.c_str(), true);
}

void AnnounceGridChargingCurrent(PubSubClient &client)
{
    AJ::JsonDocument doc;
    AddDevice(doc);

    doc["name"] = "Grid Charging Current";
    doc["unique_id"] = INSTANCE "_grid_charging_current";

    doc["state_topic"] = TOPIC_GRID_CHARGING_CURRENT_STATE;
    doc["command_topic"] = TOPIC_GRID_CHARGING_CURRENT_SET;

    doc["min"] = 0;
    doc["max"] = 60;
    doc["step"] = 1;
    doc["unit_of_measurement"] = "A";
    doc["device_class"] = "current";
    doc["entity_category"] = "config";
    doc["mode"] = "slider";
    doc["icon"] = "mdi:battery-charging-50";

    std::string payload;
    serializeJson(doc, payload);

    client.publish("homeassistant/number/" INSTANCE "/grid_charging_current/config", payload.c_str(), true);
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
        self->QueryTou();
        self->QueryMaxChargingCurrent();
        self->QueryGridChargingCurrent();
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
            _client.subscribe(INSTANCE "/+/set");
            // Once connected, publish an announcement...
        } else {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    for (const auto &sensor : SENSORS) {
        AnnounceSensor(sensor, _client);
    }
    AnnounceTou(_client);
    AnnounceMaxChargingCurrent(_client);
    AnnounceGridChargingCurrent(_client);

    //AnnounceCurrentSelect(_client);
    //AnnounceBulkVoltage(_client);
    //AnnounceFloatingVoltage(_client);
    //AnnounceDcCutoffVoltage(_client);
}

void Inverter::QueryRegisters()
{
    Log::Info("QueryRegisters\n");

    using namespace ArduinoJson;
    JsonDocument doc;

    // Group adjacent registers into bunches
    constexpr const uint16_t MAX_REGISTERS_PER_READ = 16;

    for (size_t first = 0; first < std::size(SENSORS); ) {
        size_t last = first;

        while (last + 1 < std::size(SENSORS)) {
            uint16_t prev = SENSORS[last].address;
            uint16_t next = SENSORS[last + 1].address;

            if (next - prev >= 3 || next - SENSORS[first].address + 1 > MAX_REGISTERS_PER_READ)
                break;

            ++last;
        }

        uint16_t start = SENSORS[first].address;
        uint16_t count = SENSORS[last].address - start + 1;

        if (_node.readHoldingRegisters(start, count) == _node.ku8MBSuccess) {
            for (size_t i = first; i <= last; ++i) {
                SENSORS[i].handler(SENSORS[i], _node.getResponseBuffer(SENSORS[i].address - start), doc);
            }
        }

        first = last + 1;
    }

    std::string payload;
    payload.reserve(256);
    serializeJson(doc, payload);

    _client.publish(TOPIC_SENSOR_STATE, payload.c_str());
    Log::Info(payload.c_str());
}

namespace Tou {
    constexpr uint16_t BASE = 250;
    constexpr size_t SLOTS = 6;

    constexpr uint8_t START = 0;   // 250-255
    constexpr uint8_t POWER = 6;   // 256-261
    constexpr uint8_t SOC   = 18;  // 268-273
    constexpr uint8_t FLAGS = 24;  // 274-279

    constexpr uint8_t REG_COUNT = 26;
    constexpr uint16_t CHARGE = 0x0001;
}

void Inverter::QueryTou()
{
    AJ::JsonDocument doc;
    AJ::JsonArray periods = doc.to<AJ::JsonArray>();

    if (_node.readHoldingRegisters(Tou::BASE, Tou::REG_COUNT) == _node.ku8MBSuccess) {
        for (size_t i = 0; i < Tou::SLOTS; ++i) {
            auto period = periods.add<AJ::JsonArray>();

            period.add(_node.getResponseBuffer(Tou::FLAGS + i) & Tou::CHARGE);
            period.add(_node.getResponseBuffer(Tou::START + i));
            period.add(_node.getResponseBuffer(Tou::SOC + i));
            period.add(_node.getResponseBuffer(Tou::POWER + i));
        }
    }

    std::string payload;
    serializeJson(doc, payload);

    _client.publish(TOPIC_TOU_STATE, payload.c_str(), true);
}

namespace GridCharging {
    constexpr uint16_t CURRENT = 230;
}

void Inverter::QueryGridChargingCurrent()
{
    if (_node.readHoldingRegisters(GridCharging::CURRENT, 1) == _node.ku8MBSuccess) {

        const uint16_t current = _node.getResponseBuffer(0);

        char payload[8];
        snprintf(payload, sizeof(payload), "%u", current);

        _client.publish(TOPIC_GRID_CHARGING_CURRENT_STATE, payload, true);
    }
}

namespace MaxCharging {
    constexpr uint16_t CURRENT = 210;
}

void Inverter::QueryMaxChargingCurrent()
{
    if (_node.readHoldingRegisters(MaxCharging::CURRENT, 1) == _node.ku8MBSuccess) {

        const uint16_t current = _node.getResponseBuffer(0);

        char payload[8];
        snprintf(payload, sizeof(payload), "%u", current);

        _client.publish(TOPIC_MAX_CHARGING_CURRENT_STATE, payload, true);
    }
}

void Inverter::_HandleCallback(char* topic, byte* payload, unsigned int length)
{
     if (!strcmp(topic, TOPIC_TOU_SET)) {
         AJ::JsonDocument doc;
         if (deserializeJson(doc, payload, length))
             return;

         auto table = doc.as<AJ::JsonArray>();
         if (table.size() != Tou::SLOTS)
             return;

         uint16_t start[Tou::SLOTS];
         uint16_t power[Tou::SLOTS];
         uint16_t soc[Tou::SLOTS];
         uint16_t flags[Tou::SLOTS];

         // Preserve unknown flag bits.
         if (_node.readHoldingRegisters(Tou::BASE + Tou::FLAGS, Tou::SLOTS) != _node.ku8MBSuccess)
             return;

         for (size_t i = 0; i < Tou::SLOTS; ++i)
             flags[i] = _node.getResponseBuffer(i);

         for (size_t i = 0; i < Tou::SLOTS; ++i) {
             auto row = table[i].as<AJ::JsonArray>();

             bool charge = row[0];
             start[i] = row[1];
             soc[i] = row[2];
             power[i] = row[3];

             flags[i] = (flags[i] & ~Tou::CHARGE) | (charge ? Tou::CHARGE : 0);
         }

         auto writeBlock = [this](uint16_t reg, const uint16_t *values) {
             _node.beginTransmission(reg);
             for (size_t i = 0; i < Tou::SLOTS; ++i)
                 _node.send(values[i]);
             return _node.writeMultipleRegisters(reg, Tou::SLOTS);
         };

         if (writeBlock(Tou::BASE + Tou::START, start) != _node.ku8MBSuccess) return;
         if (writeBlock(Tou::BASE + Tou::POWER, power) != _node.ku8MBSuccess) return;
         if (writeBlock(Tou::BASE + Tou::SOC, soc) != _node.ku8MBSuccess) return;
         writeBlock(Tou::BASE + Tou::FLAGS, flags);
    } else if (!strcmp(topic, TOPIC_GRID_CHARGING_CURRENT_SET)) {
        // MQTT number entities send the value as plain text.
        char buffer[12];

        if (length == 0 || length >= sizeof(buffer))
            return;

        memcpy(buffer, payload, length);
        buffer[length] = '\0';

        char *end;
        unsigned long current = strtoul(buffer, &end, 10);

        // Reject anything that wasn't a plain integer.
        if (*end != '\0' || current > UINT16_MAX)
            return;

        _node.setTransmitBuffer(0, static_cast<uint16_t>(current));

        _node.writeMultipleRegisters(GridCharging::CURRENT, 1);
    } else if (!strcmp(topic, TOPIC_MAX_CHARGING_CURRENT_SET)) {
        // MQTT number entities send the value as plain text.
        char buffer[12];

        if (length == 0 || length >= sizeof(buffer))
            return;

        memcpy(buffer, payload, length);
        buffer[length] = '\0';

        char *end;
        unsigned long current = strtoul(buffer, &end, 10);

        // Reject anything that wasn't a plain integer.
        if (*end != '\0' || current > UINT16_MAX)
            return;

        _node.setTransmitBuffer(0, static_cast<uint16_t>(current));

        _node.writeMultipleRegisters(MaxCharging::CURRENT, 1);
    }

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
