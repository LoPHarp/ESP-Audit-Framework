#pragma once

#include <cstdint>
#include <string>
#include <variant>  
#include <cstring>

struct MacAddress
{
    uint8_t addr[6];
    std::string toString() const;

    bool operator==(const MacAddress& other) const 
    {
        return memcmp(addr, other.addr, 6) == 0;
    }
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
    char ssid[33];
    uint8_t channel;

    bool operator==(const BeaconFrame& other) const
    {
        return base.source == other.base.source;
    }
};

struct ProbeRequestFrame  
{
    RawFrame base;
    char ssid[33];

    bool operator==(const ProbeRequestFrame& other) const
    {
        return base.source == other.base.source;
    }
};

using FrameVariant = std::variant<BeaconFrame, ProbeRequestFrame>;