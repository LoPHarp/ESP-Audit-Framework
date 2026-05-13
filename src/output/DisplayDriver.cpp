#include "DisplayDriver.hpp"
#include "UkrFont.hpp"
//#include "../storage/PcapWriter.hpp"
#include "driver/gpio.h"

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
    lastAnimDots_ = 255;
    lastSessionEapol_ = 0xFFFFFFFF;
}

void DisplayDriver::DrawStatusBar(string_view title, string_view time, uint32_t sessionEapol)
{
    bool titleChanged = (lastTitle_ != title);
    bool timeChanged = (lastTime_ != time);
    bool eapolChanged = (lastSessionEapol_ != sessionEapol);

    if (!titleChanged && !timeChanged && !eapolChanged) 
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
        eapolChanged = true; 
    }

    /*
    bool sdMounted = PcapWriter::GetInstance().IsMounted();
    tft_.setTextColor(sdMounted ? TFT_WHITE : TFT_RED, tft_.color565(0, 60, 0));
    tft_.setCursor(tft_.width() - 150, 3);
    tft_.print(sdMounted ? "SD" : "NO SD");
    */

    if (eapolChanged)
    {
        tft_.setTextColor(TFT_ORANGE, tft_.color565(0, 60, 0));
        tft_.setCursor(tft_.width() - 150, 3);
        tft_.printf("E:%-4lu", sessionEapol);
        lastSessionEapol_ = sessionEapol;
    }

    if (timeChanged)
    {
        tft_.setTextColor(TFT_WHITE, tft_.color565(0, 60, 0));
        int timeWidth = tft_.textWidth(string(time).c_str());
        tft_.setCursor(tft_.width() - timeWidth - 5, 3);
        tft_.print(string(time).c_str());
        lastTime_ = string(time);
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

void DisplayDriver::DrawAttackTelemetry(string_view targetMac, uint8_t channel, uint32_t packetsSent, uint32_t eapolCaught, bool forceFullRedraw)
{
    if (forceFullRedraw)
    {
        tft_.fillRect(0, 25, tft_.width(), tft_.height() - 25, TFT_BLACK);
        
        tft_.setFont(&ukrFont);
        tft_.setTextSize(1.0);
        
        tft_.setTextColor(TFT_MAGENTA, TFT_BLACK);
        tft_.setCursor(10, 40);
        tft_.print(">> АКТИВНА АТАКА: СПАМ <<");
        
        tft_.setTextColor(TFT_WHITE, TFT_BLACK);
        tft_.setCursor(10, 75);
        tft_.printf("ЦІЛЬ:  %s", string(targetMac).c_str());
        
        tft_.setCursor(10, 100);
        tft_.print("КАНАЛ: ");

        tft_.setCursor(10, 135);
        tft_.print("ВІДПРАВЛЕНО:");
        
        // --- НОВЫЙ БЛОК ---
        tft_.setCursor(10, 170);
        tft_.print("EAPOL У БУФЕРІ:");
        
        lastChannel_ = 0xFF;
    }

    if (forceFullRedraw || channel != lastChannel_)
    {
        tft_.setFont(&ukrFont);
        tft_.setTextSize(1.0);
        tft_.setTextColor(TFT_YELLOW, TFT_BLACK); 
        tft_.setCursor(65, 100); 
        tft_.printf("%-2d", channel);
        lastChannel_ = channel;
    }

    if (forceFullRedraw || packetsSent != lastPacketsSent_)
    {
        tft_.setFont(&ukrFont);
        tft_.setTextSize(1.0);
        tft_.setTextColor(TFT_GREEN, TFT_BLACK); 
        tft_.setCursor(130, 135); 
        tft_.printf("%-6lu", packetsSent); 
        
        lastPacketsSent_ = packetsSent;
    }
    
    if (forceFullRedraw || eapolCaught != lastEapolCaught_)
    {
        tft_.setFont(&ukrFont);
        tft_.setTextSize(1.0);
        tft_.setTextColor(eapolCaught > 0 ? TFT_GREEN : TFT_YELLOW, TFT_BLACK); 
        tft_.setCursor(130, 170); 
        tft_.printf("%-6lu", eapolCaught); 
        
        lastEapolCaught_ = eapolCaught;
    }
}

void DisplayDriver::DrawPmkidTelemetry(string_view targetMac, uint32_t authSent, uint32_t assocSent, bool isCaught, bool forceFullRedraw)
{
    if (forceFullRedraw)
    {
        tft_.fillRect(0, 25, tft_.width(), tft_.height() - 25, TFT_BLACK);
        
        tft_.setFont(&ukrFont);
        tft_.setTextSize(1.0);
        
        tft_.setTextColor(TFT_MAGENTA, TFT_BLACK);
        tft_.setCursor(10, 40);
        tft_.print(">> АКТИВНА АТАКА: PMKID <<");
        
        tft_.setTextColor(TFT_WHITE, TFT_BLACK);
        tft_.setCursor(10, 75);
        tft_.printf("ЦІЛЬ:  %s", string(targetMac).c_str());
        
        tft_.setCursor(10, 100);
        tft_.print("AUTH Відправлено: ");
        
        tft_.setCursor(10, 125);
        tft_.print("ASSOC Відправлено:");
        
        tft_.setCursor(10, 160);
        tft_.print("Отримано Хендшейк:");
    }

    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0);
    
    tft_.setTextColor(TFT_YELLOW, TFT_BLACK); 
    tft_.setCursor(150, 100); 
    tft_.printf("%-6lu", authSent);
    
    tft_.setCursor(150, 125); 
    tft_.printf("%-6lu", assocSent);

    tft_.setCursor(150, 160);
    if (isCaught)
    {
        tft_.setTextColor(TFT_GREEN, TFT_BLACK);
        tft_.print("ТАК  ");
        
        tft_.setCursor(10, 195);
        tft_.print(">> ЦЕ ВІКНО МОЖНА ЗАЧИНИТИ <<");
        
        uint32_t ms = pdTICKS_TO_MS(xTaskGetTickCount());
        uint16_t borderColor = (ms % 1000 < 500) ? TFT_GREEN : TFT_BLACK;
        tft_.drawRect(0, 24, tft_.width(), tft_.height() - 24, borderColor);
        tft_.drawRect(1, 25, tft_.width()-2, tft_.height() - 26, borderColor);
    }
    else
    {
        tft_.setTextColor(TFT_RED, TFT_BLACK);
        tft_.print("НІ   ");
        tft_.drawRect(0, 24, tft_.width(), tft_.height() - 24, TFT_BLACK);
        tft_.drawRect(1, 25, tft_.width()-2, tft_.height() - 26, TFT_BLACK);
    }
}

void DisplayDriver::DrawActionRow(uint8_t index, string_view text, bool isSelected, uint16_t textColor, bool isHeader)
{
    int yPos = 25 + (index * 25);
    
    uint16_t bgColor = TFT_BLACK;
    uint16_t fgColor = textColor;

    if (!isHeader && isSelected)
    {
        bgColor = TFT_GREEN;
        fgColor = TFT_BLACK;
    }

    tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    
    tft_.setTextColor(fgColor);
    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0); 

    tft_.setCursor(5, yPos + 6);
    tft_.print(string(text).c_str());
}

void DisplayDriver::DrawSettingRow(uint8_t index, string_view label, bool isActive, bool isSelected)
{
    int yPos = 25 + (index * 25);
    
    uint16_t bgColor = isSelected ? TFT_GREEN : TFT_BLACK;
    uint16_t fgColor = isSelected ? TFT_BLACK : TFT_WHITE;

    tft_.fillRect(0, yPos, tft_.width(), 25, bgColor);
    
    tft_.setFont(&ukrFont);
    tft_.setTextSize(1.0); 

    tft_.setTextColor(fgColor);
    tft_.setCursor(5, yPos + 6);
    tft_.print(string(label).c_str());

    uint16_t statusColor = isSelected ? TFT_BLACK : (isActive ? TFT_GREEN : TFT_RED);
    tft_.setTextColor(statusColor);
    
    int rightOffset = isActive ? 50 : 45;
    tft_.setCursor(tft_.width() - rightOffset, yPos + 6); 
    tft_.print(isActive ? "УВІМК" : "ВИМК");
}