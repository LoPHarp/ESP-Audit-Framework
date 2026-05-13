#include "RTCManager.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <format>

using namespace std;

constexpr uint8_t ParseCompileMonth(const char* dateStr)
{
    if (dateStr[0] == 'J' && dateStr[1] == 'a') return 1;
    if (dateStr[0] == 'F') return 2;
    if (dateStr[0] == 'M' && dateStr[2] == 'r') return 3;
    if (dateStr[0] == 'A' && dateStr[1] == 'p') return 4;
    if (dateStr[0] == 'M' && dateStr[2] == 'y') return 5;
    if (dateStr[0] == 'J' && dateStr[2] == 'n') return 6;
    if (dateStr[0] == 'J' && dateStr[2] == 'l') return 7;
    if (dateStr[0] == 'A' && dateStr[1] == 'u') return 8;
    if (dateStr[0] == 'S') return 9;
    if (dateStr[0] == 'O') return 10;
    if (dateStr[0] == 'N') return 11;
    if (dateStr[0] == 'D') return 12;
    return 1;
}

constexpr uint8_t ParseCompileDay(const char* dateStr)
{
    uint8_t tens = dateStr[4] == ' ' ? 0 : dateStr[4] - '0';
    uint8_t ones = dateStr[5] - '0';
    return tens * 10 + ones;
}

constexpr uint8_t ParseCompileYear(const char* dateStr)
{
    return (dateStr[9] - '0') * 10 + (dateStr[10] - '0');
}

constexpr uint8_t ParseCompileHour(const char* timeStr)
{
    return (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
}

constexpr uint8_t ParseCompileMinute(const char* timeStr)
{
    return (timeStr[3] - '0') * 10 + (timeStr[4] - '0');
}

constexpr uint8_t ParseCompileSecond(const char* timeStr)
{
    return (timeStr[6] - '0') * 10 + (timeStr[7] - '0');
}

RTCManager& RTCManager::GetInstance()
{
    static RTCManager instance;
    return instance;
}

void RTCManager::Initialize()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_CE) | (1ULL << PIN_CLK);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << PIN_DAT);
    gpio_config(&io_conf);

    gpio_set_level(PIN_CE, 0);
    gpio_set_level(PIN_CLK, 0);

    uint8_t magic = ReadByte(0xC1);

    if (magic != MAGIC_BYTE_VAL)
        SetupFromCompileTime();
}

string RTCManager::GetCurrentTime()
{
    uint8_t burstData[3];
    ReadBurst(burstData, 3);

    uint8_t m = BcdToDec(burstData[1] & 0x7F);
    uint8_t h = BcdToDec(burstData[2] & 0x3F);
    return format("{:02}:{:02}", h, m);
}

void RTCManager::SetupFromCompileTime()
{
    WriteByte(0x8E, 0x00);

    constexpr uint8_t s = ParseCompileSecond(__TIME__);
    constexpr uint8_t m = ParseCompileMinute(__TIME__);
    constexpr uint8_t h = ParseCompileHour(__TIME__);
    constexpr uint8_t d = ParseCompileDay(__DATE__);
    constexpr uint8_t mo = ParseCompileMonth(__DATE__);
    constexpr uint8_t y = ParseCompileYear(__DATE__);

    WriteByte(0x80, DecToBcd(s) & 0x7F);
    WriteByte(0x82, DecToBcd(m));
    WriteByte(0x84, DecToBcd(h));
    WriteByte(0x86, DecToBcd(d));
    WriteByte(0x88, DecToBcd(mo));
    WriteByte(0x8A, 1);
    WriteByte(0x8C, DecToBcd(y));

    WriteByte(0xC0, MAGIC_BYTE_VAL);
}

void RTCManager::ShiftOut(uint8_t val)
{
    gpio_set_direction(PIN_DAT, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; ++i)
    {
        gpio_set_level(PIN_DAT, (val & (1 << i)) ? 1 : 0);
        gpio_set_level(PIN_CLK, 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_CLK, 0);
        esp_rom_delay_us(1);
    }
}

uint8_t RTCManager::ShiftIn()
{
    uint8_t val = 0;
    gpio_set_direction(PIN_DAT, GPIO_MODE_INPUT);
    for (int i = 0; i < 8; ++i)
    {
        if (gpio_get_level(PIN_DAT))
            val |= (1 << i);
        gpio_set_level(PIN_CLK, 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_CLK, 0);
        esp_rom_delay_us(1);
    }
    return val;
}

void RTCManager::WriteByte(uint8_t address, uint8_t data)
{
    portENTER_CRITICAL(&rtcMux_);
    gpio_set_level(PIN_CE, 1);
    esp_rom_delay_us(2);

    gpio_set_direction(PIN_DAT, GPIO_MODE_OUTPUT);
    
    for (int i = 0; i < 8; ++i) 
    {
        gpio_set_level(PIN_DAT, (address & (1 << i)) ? 1 : 0);
        gpio_set_level(PIN_CLK, 1); esp_rom_delay_us(2);
        gpio_set_level(PIN_CLK, 0); esp_rom_delay_us(2);
    }
    
    for (int i = 0; i < 8; ++i) 
    {
        gpio_set_level(PIN_DAT, (data & (1 << i)) ? 1 : 0);
        gpio_set_level(PIN_CLK, 1); esp_rom_delay_us(2);
        gpio_set_level(PIN_CLK, 0); esp_rom_delay_us(2);
    }

    gpio_set_level(PIN_CE, 0);
    portEXIT_CRITICAL(&rtcMux_);
}

uint8_t RTCManager::ReadByte(uint8_t address)
{
    portENTER_CRITICAL(&rtcMux_);
    gpio_set_level(PIN_CE, 1);
    esp_rom_delay_us(2);

    gpio_set_direction(PIN_DAT, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; ++i) 
    {
        gpio_set_level(PIN_DAT, (address & (1 << i)) ? 1 : 0);
        gpio_set_level(PIN_CLK, 1); esp_rom_delay_us(2);
        
        if (i == 7) 
            gpio_set_direction(PIN_DAT, GPIO_MODE_INPUT);
        
        gpio_set_level(PIN_CLK, 0); esp_rom_delay_us(2);
    }

    uint8_t val = 0;
    for (int i = 0; i < 8; ++i) 
    {
        if (gpio_get_level(PIN_DAT)) val |= (1 << i);
        gpio_set_level(PIN_CLK, 1); esp_rom_delay_us(2);
        gpio_set_level(PIN_CLK, 0); esp_rom_delay_us(2);
    }

    gpio_set_level(PIN_CE, 0);
    portEXIT_CRITICAL(&rtcMux_);
    return val;
}

void RTCManager::ReadBurst(uint8_t* buffer, uint8_t count)
{
    portENTER_CRITICAL(&rtcMux_);
    gpio_set_level(PIN_CE, 1);
    esp_rom_delay_us(2);

    gpio_set_direction(PIN_DAT, GPIO_MODE_OUTPUT);
    uint8_t address = 0xBF;

    for (int i = 0; i < 8; ++i) 
    {
        gpio_set_level(PIN_DAT, (address & (1 << i)) ? 1 : 0);
        gpio_set_level(PIN_CLK, 1); esp_rom_delay_us(2);
        
        if (i == 7) 
            gpio_set_direction(PIN_DAT, GPIO_MODE_INPUT);
        
        
        gpio_set_level(PIN_CLK, 0); esp_rom_delay_us(2);
    }

    for (int byteIdx = 0; byteIdx < count; ++byteIdx) 
    {
        uint8_t val = 0;
        for (int i = 0; i < 8; ++i) 
        {
            if (gpio_get_level(PIN_DAT)) val |= (1 << i);
            gpio_set_level(PIN_CLK, 1); esp_rom_delay_us(2);
            gpio_set_level(PIN_CLK, 0); esp_rom_delay_us(2);
        }
        buffer[byteIdx] = val;
    }

    gpio_set_level(PIN_CE, 0);
    portEXIT_CRITICAL(&rtcMux_);
}

uint8_t RTCManager::DecToBcd(uint8_t val) const
{
    return ((val / 10) << 4) + (val % 10);
}

uint8_t RTCManager::BcdToDec(uint8_t val) const
{
    return ((val >> 4) * 10) + (val & 0x0F);
}