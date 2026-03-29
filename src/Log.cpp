#include "Log.h"
#include "WiFiUdp.h"

WiFiUDP udp;

void Log::Info(const char *msg)
{
    udp.beginPacket("192.168.3.214", 12345);
    udp.write((const uint8_t *)msg, strlen(msg));
    udp.endPacket();
}
