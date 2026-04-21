#pragma once 

#include "../sniffer/sniffer.hpp"

#include <vector>
#include <variant>
#include <mutex>

using namespace std;
using APVariant = variant<BeaconFrame, ProbeRequestFrame>;
using APVVector = std::vector<APVariant>;

class AccessPointManager
{
public:
    static AccessPointManager& GetInstance();
    void Initalize();

    void AddOrUpdateAccessPoint(const APVariant& frame);
    uint32_t GetDataVersion();
    APVVector GetNetworks();

private:
    AccessPointManager();
    ~AccessPointManager() = default;

    AccessPointManager(const AccessPointManager&) = delete;
    AccessPointManager operator=(const AccessPointManager&) = delete;

    uint32_t version_ = 0;
    APVVector AccessPoints;
    mutable mutex mtx_;
};