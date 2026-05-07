#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct EapolPacket
{
    uint8_t data[256];
    size_t length;
};

class HandshakeCatcher
{
public:
    static HandshakeCatcher& GetInstance();

    void Initialize();
    void ProcessEapolPacket(std::span<const uint8_t> rawPacket);
    std::vector<std::vector<uint8_t>> GetAndClearBuffer();
    
    size_t GetBufferedBytesCount() const;

    size_t GetPacketsInQueue() const;
    
private:
    HandshakeCatcher() = default;
    ~HandshakeCatcher() = default;

    HandshakeCatcher(const HandshakeCatcher&) = delete;
    HandshakeCatcher& operator=(const HandshakeCatcher&) = delete;

    QueueHandle_t eapolQueue_{nullptr};
};