#include "frame_parser.hpp"

#include <cstring>
#include <algorithm>

using namespace std;

optional<FrameVariant> FrameParser::parse(span<const uint8_t> buffer, int8_t rssi)
{
    if(buffer.size() < 24)
        return nullopt;

    uint8_t type = (buffer[0] >> 2) & 0x03;
    uint8_t subtype = (buffer[0] >> 4) & 0x0F;

    if(type != 0 && type != 2)
        return nullopt;

    if(type == 0 && (subtype != 8 && subtype !=4))
        return nullopt;

    MacAddress dst, src, bssid;
    memcpy(dst.addr, buffer.data() + 4, 6);
    memcpy(src.addr, buffer.data() + 10, 6);
    memcpy(bssid.addr, buffer.data() + 16, 6);

    RawFrame baseFrame;
    baseFrame.destination = dst;
    baseFrame.source = src;
    baseFrame.bssid = bssid;
    baseFrame.rssi = rssi;

    if(type == 2)
    {
        DataFrame data;
        data.base = baseFrame;
        return FrameVariant{data};
    }

    if(subtype == 8)
    {
        BeaconFrame beacon;
        beacon.base = baseFrame;
        memset(beacon.ssid, 0, sizeof(beacon.ssid));

        if(!parseBeaconTags(buffer.subspan(24), beacon))
            return nullopt;

        return FrameVariant{beacon};
    }
    else if(subtype == 4)
    {
        ProbeRequestFrame probe;
        probe.base = baseFrame;
        memset(probe.ssid, 0, sizeof(probe.ssid));

        if(!parseProbeRequestTags(buffer.subspan(24), probe))
            return nullopt;

        return FrameVariant{probe};
    }

    return nullopt;
}

bool FrameParser::parseBeaconTags(span<const uint8_t> payload, BeaconFrame& beacon)
{
    if(payload.size() < 16)
        return false;

    beacon.channel = 0;
    size_t offset = 12;
    size_t fcs_length = 4;

    while(offset + 1 < (payload.size() - fcs_length))
    {
        uint8_t tagLength = payload[offset + 1];

        if ((offset + 2 + tagLength) > (payload.size() - fcs_length))
            break;

        uint8_t tagNumber = payload[offset];

        if (tagNumber == 0)
        {
            size_t copyLen = min<size_t>(tagLength, 32);
            memcpy(beacon.ssid, payload.data() + offset + 2, copyLen);
            beacon.ssid[copyLen] = '\0';
        }
        else if (tagNumber == 3 && tagLength == 1)
        {
            beacon.channel = payload[offset + 2];
        }
        
        offset += 2 + tagLength;
    }

    return true;
}

bool FrameParser::parseProbeRequestTags(span<const uint8_t> payload, ProbeRequestFrame& probe)
{
    size_t offset = 0;
    size_t fcs_length = 4;

    while(offset + 1 < (payload.size() - fcs_length))
    {
        uint8_t tagLength = payload[offset + 1];

        if ((offset + 2 + tagLength) > (payload.size() - fcs_length))
            break;

        uint8_t tagNumber = payload[offset];
        
        if (tagNumber == 0)
        {
            size_t copyLen = min<size_t>(tagLength, 32);
            memcpy(probe.ssid, payload.data() + offset + 2, copyLen);
            probe.ssid[copyLen] = '\0';
        }
        
        offset += 2 + tagLength;
    }

    return true;
}