#pragma once

#include <cstdint>
#include <functional>
#include <ArduinoJson.h>

enum class Unit
{
    Percent,
    V,
    A,
    W,
    kWh,
};

inline const char* ToString(Unit u)
{
    switch (u) {
    case Unit::V: return "V";
    case Unit::A: return "A";
    case Unit::Percent: return "%";
    case Unit::W: return "W";
    case Unit::kWh: return "kWh";
    }
    return "";
}

enum class DeviceClass
{
    Battery,
    Voltage,
    Current,
    Power,
    Energy,
};

inline const char* ToString(DeviceClass c)
{
    switch (c) {
    case DeviceClass::Voltage: return "voltage";
    case DeviceClass::Current: return "current";
    case DeviceClass::Battery: return "battery";
    case DeviceClass::Power: return "power";
    case DeviceClass::Energy: return "energy";
    }
    return "";
}

enum class StateClass
{
    Measurement,
    TotalIncreasing,
};

inline const char* ToString(StateClass c)
{
    switch (c) {
    case StateClass::Measurement: return "measurement";
    case StateClass::TotalIncreasing: return "total_increasing";
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

