#include "test_manager.h"
#include "mock_frames.h"
#include "../sniffer/frame_parser.h"
#include <iostream>
#include <format>

using namespace std;

void TestManager::runAllTests()
{
    cout << "================================" << endl;
    cout << "    STARTING SYSTEM TESTS       " << endl;
    cout << "================================" << endl;

    testBeaconParsing();
    testProbeRequestParsing();

    cout << "================================" << endl;
    cout << "    TESTS COMPLETED             " << endl;
    cout << "================================" << endl;
}

void TestManager::testBeaconParsing()
{
    auto result = FrameParser::parse(mockBeaconFrame, -60);

    if (!result)
    {
        cout << "[FAILED] Beacon Parsing: Parser returned nullopt" << endl;
        return;
    }

    if (holds_alternative<BeaconFrame>(result.value()))
    {
        const auto& beacon = get<BeaconFrame>(result.value());
        cout << "[PASSED] Beacon Parsing" << endl;
        cout << format("  SSID: {}, Channel: {}, RSSI: {}", 
                       beacon.ssid, beacon.channel, beacon.base.rssi) << endl;
    }
    else 
        cout << "[FAILED] Beacon Parsing: Wrong frame type in variant" << endl;
}

void TestManager::testProbeRequestParsing()
{
    auto result = FrameParser::parse(mockProbeRequestFrame, -75);

    if (!result)
    {
        cout << "[FAILED] Probe Request Parsing: Parser returned nullopt" << endl;
        return;
    }

    if (holds_alternative<ProbeRequestFrame>(result.value()))
    {
        const auto& probe = get<ProbeRequestFrame>(result.value());
        cout << "[PASSED] Probe Request Parsing" << endl;
        cout << format("  SSID: {}, RSSI: {} dBm, Client MAC: {}", 
                       probe.ssid, probe.base.rssi, probe.base.source.toString()) << endl;
    }
    else 
    {
        cout << "[FAILED] Probe Request Parsing: Wrong frame type in variant" << endl;
    }
}