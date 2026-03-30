#pragma once

#include <cstdint>
#include <functional>
#include <ArduinoJson.h>

enum class Unit
{
    V,
    Percent,
    W,
};

inline const char* ToString(Unit u)
{
    switch (u) {
    case Unit::V: return "V";
    case Unit::Percent: return "%";
    case Unit::W: return "W";
    }
    return "";
}

enum class DeviceClass
{
    Voltage,
    Battery,
    Power,
};

inline const char* ToString(DeviceClass c)
{
    switch (c) {
    case DeviceClass::Voltage: return "voltage";
    case DeviceClass::Battery: return "battery";
    case DeviceClass::Power: return "power";
    }
    return "";
}

enum class StateClass
{
    Measurement,
};

inline const char* ToString(StateClass c)
{
    switch (c) {
    case StateClass::Measurement: return "measurement";
    }
    return "";
}

struct Sensor
{
    using HandlerT = std::function<void(const Sensor &, const uint16_t *, ArduinoJson::JsonDocument &)>;

    uint16_t address;
    const char *id;
    const char *name;
    Unit unit_of_measurement;
    DeviceClass device_class;
    StateClass state_class;
    int accuracy_decimals;
    const char *icon;
    HandlerT handler;
};

