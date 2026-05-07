#include "AccessPointManager.hpp"

#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace std;

AccessPointManager& AccessPointManager::GetInstance()
{
    static AccessPointManager Instance;
    return Instance;
}

void AccessPointManager::Initialize()
{
    scoped_lock lock(mtx_);
    accessPoints_.reserve(100);
    stations_.reserve(200);
}

void AccessPointManager::AddOrUpdateAP(const MacAddress& bssid, const char* ssid, int8_t rssi, uint8_t channel, const SecurityInfo& sec)
{
    scoped_lock lock(mtx_);

    auto it = find_if(accessPoints_.begin(), accessPoints_.end(), [&bssid](const AccessPoint& existing) {
        return existing.bssid == bssid;
    });

    if (it != accessPoints_.end())
    {
        it->rssi = rssi;
        it->lastSeen = xTaskGetTickCount();
        it->channel = channel;
        it->security = sec;
        if(ssid[0] != '\0')
        {
            strncpy(it->ssid, ssid, 32);
            it->ssid[32] = '\0';
        }
    }
    else if (accessPoints_.size() < 100)
    {
            AccessPoint newPoint;
            newPoint.bssid = bssid;
            newPoint.channel = channel;
            newPoint.lastSeen = xTaskGetTickCount();
            newPoint.rssi = rssi;
            newPoint.security = sec;
            strncpy(newPoint.ssid, ssid, 32);
            newPoint.ssid[32] = '\0';
            accessPoints_.push_back(newPoint);
    }
    else
        return;

    version_++;
}

void AccessPointManager::AddOrUpdateStation(const MacAddress& stationMac, const MacAddress& bssid, int8_t rssi)
{
    scoped_lock lock(mtx_);

    auto it = find_if(stations_.begin(), stations_.end(), [stationMac](const Station& existing) {
        return existing.mac == stationMac;
    });

    if (it != stations_.end())
    {
        it->rssi = rssi;
        it->lastSeen = xTaskGetTickCount();
        MacAddress emptyMac = {0};
        if(!(bssid == emptyMac))
            it->bssid = bssid;
    }
    else if (stations_.size() < 200)
    {
            Station newStation;
            newStation.mac = stationMac;
            newStation.rssi = rssi;
            newStation.bssid = bssid;
            newStation.lastSeen = xTaskGetTickCount();
            stations_.push_back(newStation);
    }
    else
        return;

    version_++;
}

uint32_t AccessPointManager::GetDataVersion() const
{
    scoped_lock lock(mtx_);
    return version_;
}

vector<AccessPoint> AccessPointManager::GetAccessPoints()
{
    vector<AccessPoint> copy;
    
    {
        scoped_lock lock(mtx_);
        copy = accessPoints_; 
    }

    sort(copy.begin(), copy.end(), [](const AccessPoint& a, const AccessPoint& b){
        return a.rssi > b.rssi; 
    });

    return copy;
}

vector<Station> AccessPointManager::GetAllStations()
{
    vector<Station> copy;
    {
        scoped_lock lock(mtx_);
        copy = stations_;
    }
    
    sort(copy.begin(), copy.end(), [](const Station& a, const Station& b){
        return a.rssi > b.rssi;
    });

    return copy;
}

vector<Station> AccessPointManager::GetStationsForAP(const MacAddress& bssid)
{
    vector<Station> result;
    
    {
        scoped_lock lock(mtx_);
        for (const auto& st : stations_)
        {
            if (st.bssid == bssid)
                result.push_back(st);
        }
    }

    sort(result.begin(), result.end(), [](const Station& a, const Station& b){
        return a.rssi > b.rssi;
    });

    return result;
}