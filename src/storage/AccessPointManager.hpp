#pragma once 

#include <cstdint>
#include <vector>
#include <mutex>
#include "../sniffer/sniffer.hpp"

struct Station
{
    MacAddress mac;
    MacAddress bssid;
    int8_t rssi;
    uint32_t lastSeen;
};

struct AccessPoint
{
    MacAddress bssid;
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
    uint32_t lastSeen;
};

class AccessPointManager
{
public:
    static AccessPointManager& GetInstance();
    void Initialize();

    void AddOrUpdateAP(const MacAddress& bssid, const char* ssid, int8_t rssi, uint8_t channel);
    void AddOrUpdateStation(const MacAddress& stationMac, const MacAddress& bssid, int8_t rssi);

    std::vector<AccessPoint> GetAccessPoints();
    std::vector<Station> GetStationsForAP(const MacAddress& bssid);
    std::vector<Station> GetAllStations();

    uint32_t GetDataVersion() const;

private:
    AccessPointManager() = default;
    ~AccessPointManager() = default;

    AccessPointManager(const AccessPointManager&) = delete;
    AccessPointManager& operator=(const AccessPointManager&) = delete;

    std::vector<AccessPoint> accessPoints_;
    std::vector<Station> stations_;

    mutable std::mutex mtx_;
    uint32_t version_ = 0;
};