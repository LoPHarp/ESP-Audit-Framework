#include "DisplayDriver.hpp"
#include "UkrFont.hpp"

using namespace std;

static const lgfx::U8g2font ukrFont(u8g2_font_unifont_t_cyrillic);

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

void DisplayDriver::ResetState()
{
    lastTitle_ = "";
    lastTime_ = "";
    lastBattery_ = -1.0f;
    lastAnimDots_ = 255;
}

void DisplayDriver::DrawStatusBar(string_view title, float battery, string_view time)
{
    bool titleChanged = (lastTitle_ != title);
    bool timeChanged = (lastTime_ != time);
    bool batChanged = (abs(lastBattery_ - battery) > 0.01f);

    if (!titleChanged && !timeChanged && !batChanged) 
        return;

    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0);

    if (titleChanged)
    {
        tft_.fillRect(0, 0, tft_.width(), 22, tft_.color565(0, 60, 0));
        tft_.drawFastHLine(0, 22, tft_.width(), TFT_GREEN);
        tft_.setTextColor(TFT_WHITE, tft_.color565(0, 60, 0));
        tft_.setCursor(5, 3);
        tft_.print(string(title).c_str());
        lastTitle_ = string(title);
        timeChanged = true; 
        batChanged = true;
    }

    if (timeChanged)
    {
        tft_.setTextColor(TFT_WHITE, tft_.color565(0, 60, 0));
        int timeWidth = tft_.textWidth(string(time).c_str());
        tft_.setCursor((tft_.width() - timeWidth) / 2, 3);
        tft_.print(string(time).c_str());
        lastTime_ = string(time);
    }

    if (batChanged)
    {
        int batPercent = (int)(battery * 100);
        uint16_t batColor = (batPercent < 20) ? TFT_RED : (batPercent < 50 ? TFT_YELLOW : TFT_GREEN);
        tft_.setTextColor(batColor, tft_.color565(0, 60, 0));
        tft_.setCursor(tft_.width() - 55, 3);
        tft_.printf("%3d%%", batPercent); // %3d затирает старые символы
        tft_.drawRect(tft_.width() - 20, 5, 15, 10, batColor);
        tft_.fillRect(tft_.width() - 20, 5, (int)(15 * battery), 10, batColor);
        lastBattery_ = battery;
    }
}

void DisplayDriver::DrawMenuRow(uint8_t index, string_view text, bool isSelected)
{
    int yPos = 25 + (index * 25);

    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_GREEN;

    tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    
    tft_.setTextColor(fgColor);
    
    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0); 

    tft_.setCursor(5, yPos + 6);
    tft_.print(std::string(text).c_str());
}

void DisplayDriver::DrawAPRow(uint8_t index, string_view ssid, uint8_t channel, size_t clients, int8_t rssi, bool isSelected, bool forceFullRedraw)
{
    int yPos = 25 + (index * 25);
    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_WHITE;

    if (forceFullRedraw)
    {
        tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    }

    tft_.setFont(&ukrFont);
    tft_.setTextColor(fgColor, bgColor);

    tft_.setCursor(5, yPos + 6);
    string safeSsid = ssid.empty() ? "<Прихована>" : string(ssid.substr(0, 15));
    tft_.printf("%-15s", safeSsid.c_str());

    tft_.setCursor(155, yPos + 6);
    tft_.printf("Кан:%-2d Кл:%-2d", channel, clients);

    uint16_t rssiColor = isSelected ? TFT_BLACK : (rssi < -80 ? TFT_RED : (rssi < -65 ? TFT_YELLOW : TFT_GREEN));
    tft_.setTextColor(rssiColor, bgColor);
    tft_.setCursor(280, yPos + 6);
    tft_.printf("%3d", rssi);
}

void DisplayDriver::DrawAPClientRow(uint8_t index, string_view mac, uint32_t lastSeenTick, int8_t rssi, bool isSelected, bool forceFullRedraw)
{
    int yPos = 25 + (index * 25);
    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_WHITE;

    if (forceFullRedraw)
    {
        tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    }

    tft_.setFont(&ukrFont);
    tft_.setTextColor(fgColor, bgColor);
    
    tft_.setCursor(5, yPos + 6);
    tft_.printf("%-17s", string(mac).c_str()); 

    tft_.setCursor(160, yPos + 6);
    uint32_t currentTick = xTaskGetTickCount();
    uint32_t ageSeconds = (currentTick - lastSeenTick) / configTICK_RATE_HZ;
    
    if (ageSeconds < 60)
        tft_.printf("Актив:%-2luс ", ageSeconds);
    else if (ageSeconds < 3600)
        tft_.printf("Актив:%-2luхв", ageSeconds / 60);
    else
        tft_.printf("Давно...   ");

    uint16_t rssiColor = isSelected ? TFT_BLACK : (rssi < -80 ? TFT_RED : (rssi < -65 ? TFT_YELLOW : TFT_GREEN));
    tft_.setTextColor(rssiColor, bgColor);
    tft_.setCursor(285, yPos + 6);
    tft_.printf("%3d", rssi);
}

void DisplayDriver::DrawStationRow(uint8_t index, string_view mac, string_view apSsid, int8_t rssi, bool isSelected, bool forceFullRedraw)
{
    int yPos = 25 + (index * 25);
    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_WHITE; 

    if (forceFullRedraw)
    {
        tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    }

    tft_.setFont(&ukrFont); 
    tft_.setTextColor(fgColor, bgColor);
    
    tft_.setCursor(5, yPos + 6);
    tft_.print(string(mac).c_str());

    tft_.setCursor(155, yPos + 6);
    string safeSsid = apSsid.empty() ? "[Шукає...]" : string(apSsid.substr(0, 26));
    tft_.printf("%-14s", safeSsid.c_str());

    uint16_t rssiColor = isSelected ? TFT_BLACK : (rssi < -80 ? TFT_RED : (rssi < -65 ? TFT_YELLOW : TFT_GREEN));
    
    tft_.setTextColor(rssiColor, bgColor);
    tft_.setCursor(285, yPos + 6);
    tft_.printf("%3d", rssi);
}

void DisplayDriver::DrawSearchingAnimation(uint8_t dots, uint8_t rowIndex)
{
    if (dots == lastAnimDots_) 
        return;
    lastAnimDots_ = dots;

    int yPos = 25 + (rowIndex * 25);
    
    tft_.fillRect(0, yPos, tft_.width(), 25, TFT_BLACK);
    
    tft_.setTextColor(TFT_WHITE, TFT_BLACK);
    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0);
    
    string text = "Пошук мереж";
    for(uint8_t i = 0; i < dots; ++i) text += ".";

    int textWidth = tft_.textWidth("Пошук мереж...");
    int xPos = (tft_.width() - textWidth) / 2;

    tft_.setCursor(xPos, yPos + 6); 
    tft_.print(text.c_str());
}