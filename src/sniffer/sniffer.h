#pragma once

#include <cstdint>
#include <string>
#include <variant>

struct MacAddress
{
    uint8_t addr[6];
    std::string toString() const;
};

struct RawFrame
{
    MacAddress bssid;
    MacAddress source;
    MacAddress destination;
    int8_t rssi;
};

struct BeaconFrame          // Фрейм, який розсилає точка доступу для оголошення своєї присутності
{
    RawFrame base;
    std::string ssid;
    uint8_t channel;
};

struct ProbeRequestFrame    // Фрейм, який надсилає клієнт для пошуку доступних мереж Wi-Fi
{
    RawFrame base;
    std::string ssid;
};

using FrameVariant = std::variant<BeaconFrame, ProbeRequestFrame>;