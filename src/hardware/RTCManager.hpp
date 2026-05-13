#pragma once

#include <cstdint>
#include <string>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>

class RTCManager
{
public:
    static RTCManager& GetInstance();
    void Initialize();
    std::string GetCurrentTime();

private:
    RTCManager() = default;
    ~RTCManager() = default;
    RTCManager(const RTCManager&) = delete;
    RTCManager& operator=(const RTCManager&) = delete;

    void WriteByte(uint8_t address, uint8_t data);
    uint8_t ReadByte(uint8_t address);
    void ReadBurst(uint8_t* buffer, uint8_t count);
    
    uint8_t DecToBcd(uint8_t val) const;
    uint8_t BcdToDec(uint8_t val) const;

    void ShiftOut(uint8_t val);
    uint8_t ShiftIn();

    void SetupFromCompileTime();

    static constexpr gpio_num_t PIN_CE = GPIO_NUM_27;
    static constexpr gpio_num_t PIN_DAT = GPIO_NUM_21;
    static constexpr gpio_num_t PIN_CLK = GPIO_NUM_22;
    static constexpr uint8_t MAGIC_BYTE_VAL = 0x5B;

    portMUX_TYPE rtcMux_ = portMUX_INITIALIZER_UNLOCKED;
};