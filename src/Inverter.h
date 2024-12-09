#pragma once

#include <PubSubClient.h>
#include <arduino-timer.h>

class WiFiClient;
class ModbusMaster;

class Inverter
{
public:
    Inverter(WiFiClient &, ModbusMaster &);

    void Setup();
    void Handle();

private:
    WiFiClient &_wifi;
    ModbusMaster &_node;
    PubSubClient _client;
    Timer<> _timer = timer_create_default();
    Timer<>::Task _task = {};

    void Reconnect();
    void QueryRegisters();
};
