#pragma once

class TestManager
{
public:
    static void runAllTests();

private:
    static void testBeaconParsing();
    static void testProbeRequestParsing();
};