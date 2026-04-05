#pragma once

#include "sniffer.h"

#include <span>
#include <optional>
#include <cstdint>

class FrameParser
{
public:
    static std::optional<FrameVariant> parse(std::span<const uint8_t> buffer, int8_t rssi);

private:
    static bool parseBeaconTags(std::span<const uint8_t> payload, BeaconFrame& beacon); 
    static bool parseProbeRequestTags(std::span<const uint8_t> payload, ProbeRequestFrame& probe);
};