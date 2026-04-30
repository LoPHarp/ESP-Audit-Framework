#pragma once

#include <LovyanGFX.hpp>
#include <string_view>
#include <cstdint>

class LGFX_Config_ILI9341 : public lgfx::LGFX_Device 
{
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;

public:
    LGFX_Config_ILI9341() 
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI3_HOST;     
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;   
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = false;
        cfg.use_lock   = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO; 
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 23;
        cfg.pin_miso = 19;
        cfg.pin_dc   = 2;
        _bus_instance.config(cfg);

         _panel_instance.setBus(&_bus_instance);

        auto pcfg = _panel_instance.config();
        pcfg.pin_cs   = 15;     
        pcfg.pin_rst  = 4;
        pcfg.pin_busy = -1;
        pcfg.panel_width  = 240;
        pcfg.panel_height = 320;
        pcfg.offset_x     = 0;
        pcfg.offset_y     = 0;
        pcfg.offset_rotation = 0;
        pcfg.dummy_read_pixel = 8;
        pcfg.dummy_read_bits  = 1;
        pcfg.readable  = true;
        pcfg.invert    = false;
        pcfg.rgb_order = false;
        pcfg.dlen_16bit = false;
        pcfg.bus_shared = true;
        _panel_instance.config(pcfg);

        setPanel(&_panel_instance);
    }
};

class DisplayDriver
{
public:
    static DisplayDriver& GetInstance();

    void Initialize();
    void ClearScreen();
    void ResetState();

    void DrawStatusBar(std::string_view title, float battery, std::string_view time);
    void DrawMenuRow(uint8_t index, std::string_view text, bool isSelected);
    void DrawAPRow(uint8_t index, std::string_view ssid, uint8_t channel, size_t clients, int8_t rssi, bool isSelected, bool forceFullRedraw);
    void DrawAPClientRow(uint8_t index, std::string_view mac, uint32_t lastSeenTick, int8_t rssi, bool isSelected, bool forceFullRedraw);
    void DrawStationRow(uint8_t index, std::string_view mac, std::string_view apSsid, int8_t rssi, bool isSelected, bool forceFullRedraw);
    void DrawSearchingAnimation(uint8_t dots, uint8_t rowIndex);
    
    void DrawAttackTelemetry(std::string_view targetMac, uint8_t channel, uint32_t packetsSent, bool forceFullRedraw);

private:
    DisplayDriver();
    ~DisplayDriver() = default;

    DisplayDriver(const DisplayDriver&) = delete;
    DisplayDriver& operator=(const DisplayDriver&) = delete;

    std::string lastTitle_;
    std::string lastTime_;
    float lastBattery_ = -1.0f;
    uint8_t lastAnimDots_ = 255;

    LGFX_Config_ILI9341 tft_;

    uint32_t lastPacketsSent_ = 0xFFFFFFFF;
    uint8_t lastChannel_ = 0xFF;
};