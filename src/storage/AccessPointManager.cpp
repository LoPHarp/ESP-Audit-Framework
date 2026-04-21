#include "AccessPointManager.hpp"
#include <algorithm>

AccessPointManager& AccessPointManager::GetInstance()
{
    static AccessPointManager Instance;
    return Instance;
}

AccessPointManager::AccessPointManager() {}

void AccessPointManager::Initalize()
{
    scoped_lock lock(mtx_);
    AccessPoints.reserve(100);
}

void AccessPointManager::AddOrUpdateAccessPoint(const APVariant& frame)
{
    scoped_lock lock(mtx_);

    auto get_mac = [](const auto& f) { return f.base.source; };
    MacAddress newMac = visit(get_mac, frame);

    auto it = find_if(AccessPoints.begin(), AccessPoints.end(), [&](const APVariant& existing) {
        return visit(get_mac, existing) == newMac;
    });

    if (it != AccessPoints.end())
        *it = frame;
    else if (AccessPoints.size() < 100)
        AccessPoints.push_back(frame);

    version_++;
}

APVVector AccessPointManager::GetNetworks()
{
    scoped_lock lock(mtx_);
    
    APVVector sorted = AccessPoints;

    sort(sorted.begin(), sorted.end(), [](const APVariant& a, const APVariant& b) {
        auto get_rssi = [](const auto& f) { return f.base.rssi; };
        return visit(get_rssi, a) > visit(get_rssi, b);
    });

    return sorted;
}

uint32_t AccessPointManager::GetDataVersion()
{
    scoped_lock lock(mtx_);
    return version_;
}