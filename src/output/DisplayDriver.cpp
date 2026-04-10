#include "DisplayDriver.hpp"

using namespace std;

DisplayDriver& DisplayDriver::GetInstance()
{
    static DisplayDriver instance;
    return instance;
}

DisplayDriver::DisplayDriver() {}

void DisplayDriver::Initialize()
{
    tft_.init();
    tft_.setRotation(1);
    tft_.setColorDepth(16);
    tft_.fillScreen(TFT_BLACK);
}

void DisplayDriver::ClearScreen()
{
    tft_.fillScreen(TFT_BLACK);
}

void DisplayDriver::DrawHeader(std::string_view title)
{
    tft_.fillRect(0, 0, tft_.width(), 20, tft_.color565(0, 100, 0));
    tft_.setTextColor(TFT_WHITE);
    tft_.setTextSize(2);
    tft_.setCursor(5, 2);

    tft_.printf("%.*s", static_cast<int>(title.length()), title.data());

    tft_.drawFastHLine(0, 20, tft_.width(), TFT_GREEN);
}

void DisplayDriver::DrawListItem(uint8_t index, string_view ssid, string_view mac, int8_t rssi, bool isSelected)
{
    int yPos = 25 + (index * 25);

    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_GREEN;

    tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    
    tft_.setTextColor(fgColor);
    tft_.setTextSize(2);
    tft_.setCursor(5, yPos + 4);
    
    tft_.printf("%-15.*s", static_cast<int>(ssid.length()), ssid.data());

    // 6 уровней сигнала вместо 4
    uint8_t bars = 0;
    if (rssi > -50) bars = 6;
    else if (rssi > -60) bars = 5;
    else if (rssi > -70) bars = 4;
    else if (rssi > -80) bars = 3;
    else if (rssi > -90) bars = 2;
    else if (rssi > -95) bars = 1;

    int xRssi = tft_.width() - 45; 
    
    for (int i = 0; i < 6; ++i) 
    {
        uint16_t color;
        if (i >= bars) 
        {
            color = isSelected ? tft_.color565(50, 50, 50) : tft_.color565(100, 100, 100);
        } 
        else 
        {
            if (isSelected) 
            {
                color = TFT_BLACK;
            } 
            else 
            {
                if (i < 2) color = TFT_RED;
                else if (i < 4) color = TFT_YELLOW;
                else color = TFT_GREEN;
            }
        }

        int barHeight = 4 + (i * 3); 
        tft_.fillRect(xRssi + (i * 6), yPos + 20 - barHeight, 4, barHeight, color);
    }
}