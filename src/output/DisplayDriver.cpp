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

void DisplayDriver::DrawHeader(std::string_view title)
{
    tft_.fillRect(0, 0, tft_.width(), 20, tft_.color565(0, 100, 0));
    tft_.setTextColor(TFT_WHITE);

    tft_.setFont(&ukrFont); 
    tft_.setTextSize(1.0);
    
    tft_.setCursor(5, 3);
    
    tft_.print(std::string(title).c_str());

    tft_.drawFastHLine(0, 20, tft_.width(), TFT_GREEN);
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

void DisplayDriver::DrawNetworkRow(uint8_t index, string_view ssid, string_view mac, int8_t rssi, bool isSelected)
{
    int yPos = 25 + (index * 25);

    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_GREEN;

    tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    tft_.setTextColor(fgColor);
    tft_.setFont(&ukrFont); 
    tft_.setTextSize(1.0);  
    
    string safeSsid = "";
    if(ssid.empty())
        safeSsid = "<Прихована>";
    else
    {
        size_t len = min<size_t>(ssid.length(), 11);
        safeSsid = string(ssid.data(), len);
    }
    
    tft_.setCursor(5, yPos + 6);
    tft_.print(safeSsid.c_str());

    tft_.setCursor(125, yPos + 6);
    tft_.print(string(mac).c_str());

    int boxX = 270;
    int boxY = yPos + 4;
    
    uint16_t borderColor = isSelected ? TFT_BLACK : TFT_WHITE;
    tft_.drawRect(boxX, boxY, 45, 17, borderColor);
    
    uint16_t rssiColor;
    if(rssi > -60) rssiColor = TFT_GREEN;
    else if(rssi > -80) rssiColor = TFT_YELLOW;
    else rssiColor = TFT_RED;

    int fillWidth = (rssi + 100) * 43 / 60;
    if(fillWidth < 0) fillWidth = 0;
    if(fillWidth > 43) fillWidth = 43;
    
    if(fillWidth > 0)
        tft_.fillRect(boxX + 1, boxY + 1, fillWidth, 15, rssiColor);
        
    tft_.setTextColor(tft_.color565(200, 200, 200));
    tft_.setCursor(boxX + 6, boxY + 1);
    tft_.printf("%d", rssi);
}

void DisplayDriver::DrawSearchingAnimation(uint8_t dots)
{
    tft_.fillRect(0, 40, tft_.width(), 30, TFT_BLACK);
    
    tft_.setTextColor(TFT_WHITE);
    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0);
    
    string text = "Пошук мереж";
    for(uint8_t i = 0; i < dots; ++i)
        text += ".";

    int textWidth = tft_.textWidth(text.c_str());
    int xPos = (tft_.width() - textWidth) / 2;

    tft_.setCursor(xPos, 50);
    tft_.print(text.c_str());
}