#include "MenuController.hpp"
#include "../sniffer/wifi_sniffer.hpp"

using namespace std;

MenuController& MenuController::GetInstance()
{
    static MenuController instance;
    return instance;
}

void MenuController::Initialize()
{
    currentState_ = MenuState::Main;
    selectedIndex_ = 0;
    lastSelectedIndex_ = 255; 
    ChangeState(MenuState::Main);
}

void MenuController::ProcessInput()
{
    InputEvent event = InputManager::GetInstance().GetEvent();

    if(event == InputEvent::None) return;

    switch(event)
    {
        case(InputEvent::Up):
        {
            if(currentMenuSize_ > 0)
            {
                if(selectedIndex_ > 0)
                    selectedIndex_--;
                else
                    selectedIndex_ = currentMenuSize_ - 1;

                if (selectedIndex_ < viewOffset_)
                    viewOffset_ = selectedIndex_;
                else if (selectedIndex_ == currentMenuSize_ - 1)
                    viewOffset_ = (currentMenuSize_ > 8) ? currentMenuSize_ - 8 : 0;
            }
            break;
        }
        case(InputEvent::Down):
        {
            if(currentMenuSize_ > 0)
            {
                selectedIndex_ = (selectedIndex_ + 1) % currentMenuSize_;

                if (selectedIndex_ >= viewOffset_ + 8)
                    viewOffset_ = selectedIndex_ - 7;
                else if (selectedIndex_ == 0)
                    viewOffset_ = 0;
            }
            break;
        }
        case(InputEvent::Select):
        {
            if(currentState_ == MenuState::Main)
            {
                switch (selectedIndex_)
                {
                    case 0: 
                        WifiSniffer::GetInstance().Start();
                        ChangeState(MenuState::Recon_AP_List); 
                        break;
                    case 1:
                        WifiSniffer::GetInstance().Start();
                        ChangeState(MenuState::Recon_Station_List);
                        break;
                    case 2: ChangeState(MenuState::Attack_Spam_Menu); break;
                    case 3: ChangeState(MenuState::Sniffer_Live); break;
                    case 4: ChangeState(MenuState::Settings_Main); break;
                }
            }
            else if (currentState_ == MenuState::Recon_AP_List)
            {
                auto aps = AccessPointManager::GetInstance().GetAccessPoints();
                if (selectedIndex_ + viewOffset_ < aps.size())
                {
                    selectedBSSID_ = aps[selectedIndex_ + viewOffset_].bssid;
                    ChangeState(MenuState::Recon_AP_Clients);
                }
            }
            else if (currentState_ == MenuState::Recon_AP_Clients)
            {
                // На кого націлені вектори
                if (selectedIndex_ + viewOffset_ == 0) {
                    // Користувач натиснув на саму точку доступу (Індекс 0)
                    // TODO ChangeState(MenuState::Target_Action_Menu); // У майбутньому
                } else {
                    // TODO Користувач натиснув на клієнта
                }
            }
            break;
        }
        case(InputEvent::Back):
        {
            if (currentState_ == MenuState::Recon_AP_Clients)
            {
                ChangeState(MenuState::Recon_AP_List);
            }
            else if (currentState_ != MenuState::Main)
            {
                WifiSniffer::GetInstance().Stop();
                ChangeState(MenuState::Main);
            }
            break;
        }
        default: break;
    }
}

void MenuController::ChangeState(MenuState newState)
{
    currentState_ = newState;
    selectedIndex_ = 0;
    lastSelectedIndex_ = 255;
    viewOffset_ = 0;
    lastViewOffset_ = 0;
    currentMenuSize_ = 0;
    lastMenuSize_ = 0;
    DisplayDriver::GetInstance().ClearScreen();
    DisplayDriver::GetInstance().ResetState();
}

void MenuController::Update()
{
    auto& db = AccessPointManager::GetInstance();
    auto& disp = DisplayDriver::GetInstance();
    
    uint32_t currentDataVersion = db.GetDataVersion();
    uint32_t currentTick = xTaskGetTickCount();
    
    static uint32_t lastUiUpdateTick = 0;
    
    bool cursorMoved = (selectedIndex_ != lastSelectedIndex_);
    bool dataChanged = (currentDataVersion != lastDataVersion_);
    bool timeToUpdate = (currentTick - lastUiUpdateTick) > pdMS_TO_TICKS(1000); 

    if (!cursorMoved && !(dataChanged && timeToUpdate)) 
        return;

    switch (currentState_)
    {
        case MenuState::Main:
            disp.DrawStatusBar("ГОЛОВНЕ МЕНЮ", 0.85f, "18:25");
            RenderMainMenu();
            break;

        case MenuState::Recon_AP_List:
            disp.DrawStatusBar("ТОЧКИ ДОСТУПУ", 0.85f, "18:25");
            RenderReconAPList();
            break;

        case MenuState::Recon_AP_Clients:
            RenderReconAPClients();
            break;

        case MenuState::Recon_Station_List:
            disp.DrawStatusBar("ГЛОБАЛЬНИЙ ПОШУК", 0.85f, "18:25");
            RenderReconStationList();
            break;
            
        default:
            break;
    }

    lastSelectedIndex_ = selectedIndex_;
    lastViewOffset_ = viewOffset_; 
    lastMenuSize_ = currentMenuSize_;
    
    if (dataChanged && timeToUpdate) 
    {
        lastDataVersion_ = currentDataVersion;
        lastUiUpdateTick = currentTick;
    }
}

void MenuController::RenderMainMenu()
{
    vector<string_view> items = {
        "Пошук мереж (AP)",
        "Пошук пристроїв (STA)",
        "Масові атаки",
        "Сніфер",
        "Налаштування"
    };

    currentMenuSize_ = items.size();

    for (size_t i = 0; i < items.size(); ++i)
    {
        DisplayDriver::GetInstance().DrawMenuRow(i, items[i], (i == selectedIndex_));
    }
}

void MenuController::RenderReconAPList()
{
    auto& db = AccessPointManager::GetInstance();
    auto& disp = DisplayDriver::GetInstance();
    
    auto aps = db.GetAccessPoints();
    currentMenuSize_ = aps.size();

    if (aps.empty())
    {
        disp.DrawSearchingAnimation( (pdTICKS_TO_MS(xTaskGetTickCount()) / 1000) % 4, 0 );
        return;
    }

    for (uint8_t i = 0; i < 8 && (i + viewOffset_) < aps.size(); ++i)
    {
        auto& ap = aps[i + viewOffset_];

        size_t absoluteIndex = i + viewOffset_;
        bool isSelected = (absoluteIndex == selectedIndex_);
        
        bool cursorMoved = (selectedIndex_ != lastSelectedIndex_);
        bool viewMoved = (viewOffset_ != lastViewOffset_);
        bool isNewRow = (absoluteIndex >= lastMenuSize_);

        bool force = (lastSelectedIndex_ == 255) || viewMoved || isNewRow || 
                     (isSelected && cursorMoved) || (absoluteIndex == lastSelectedIndex_ && cursorMoved);

        size_t clientCount = db.GetStationsForAP(ap.bssid).size();
        disp.DrawAPRow(i, ap.ssid, ap.channel, clientCount, ap.rssi, isSelected, force);
    }
}

void MenuController::RenderReconAPClients()
{
    auto& db = AccessPointManager::GetInstance();
    auto& disp = DisplayDriver::GetInstance();
    
    auto clients = db.GetStationsForAP(selectedBSSID_);
    
    AccessPoint currentAP;
    string apName = "Unknown";
    for(auto& ap : db.GetAccessPoints()) {
        if(ap.bssid == selectedBSSID_) {
            currentAP = ap;
            apName = ap.ssid;
            break;
        }
    }

    disp.DrawStatusBar(apName, 0.85f, "18:25");

    currentMenuSize_ = clients.size() + 1;

    for (uint8_t i = 0; i < 8 && (i + viewOffset_) < currentMenuSize_; ++i)
    {
        size_t actualIndex = i + viewOffset_;
        size_t absoluteIndex = i + viewOffset_;
        bool isSelected = (absoluteIndex == selectedIndex_);
        
        bool cursorMoved = (selectedIndex_ != lastSelectedIndex_);
        bool viewMoved = (viewOffset_ != lastViewOffset_);
        bool isNewRow = (absoluteIndex >= lastMenuSize_);

        bool force = (lastSelectedIndex_ == 255) || viewMoved || isNewRow || 
                     (isSelected && cursorMoved) || (absoluteIndex == lastSelectedIndex_ && cursorMoved);

        
        if (actualIndex == 0)
        {
            disp.DrawStationRow(i, currentAP.bssid.toString(), " [ЦІЛЬОВА ТД] ", currentAP.rssi, isSelected, force);
        }
        else
        {
            auto& st = clients[actualIndex - 1];
            disp.DrawAPClientRow(i, st.mac.toString(), st.lastSeen, st.rssi, isSelected, force);
        }
    }

    if (clients.empty())
        disp.DrawSearchingAnimation( (pdTICKS_TO_MS(xTaskGetTickCount()) / 1000) % 4, 1 );
}

void MenuController::RenderReconStationList()
{
    auto& db = AccessPointManager::GetInstance();
    auto& disp = DisplayDriver::GetInstance();
    
    auto allStations = db.GetAllStations();
    currentMenuSize_ = allStations.size();

    for (uint8_t i = 0; i < 8 && (i + viewOffset_) < allStations.size(); ++i)
    {
        auto& st = allStations[i + viewOffset_];
        size_t absoluteIndex = i + viewOffset_;
        bool isSelected = (absoluteIndex == selectedIndex_);
        
        bool cursorMoved = (selectedIndex_ != lastSelectedIndex_);
        bool viewMoved = (viewOffset_ != lastViewOffset_);
        bool isNewRow = (absoluteIndex >= lastMenuSize_);

        bool force = (lastSelectedIndex_ == 255) || viewMoved || isNewRow || 
                     (isSelected && cursorMoved) || (absoluteIndex == lastSelectedIndex_ && cursorMoved);


        string apName = "[Шукає...]";
        auto aps = db.GetAccessPoints();
        for(auto& ap : aps)
        {
            if(ap.bssid == st.bssid)
            {
                apName = ap.ssid;
                break;
            }
        }

        disp.DrawStationRow(i, st.mac.toString(), apName, st.rssi, isSelected, force);
    }
}