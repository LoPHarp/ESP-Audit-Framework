#include "MenuController.hpp"
#include "../sniffer/wifi_sniffer.hpp"
#include "../attacks/DeauthManager.hpp"
#include "../attacks/BeaconSpamManager.hpp"

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
            switch (currentState_)
            {
                case MenuState::Main:
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
                        case 2:
                            WifiSniffer::GetInstance().Start(); 
                            ChangeState(MenuState::Mass_Attacks_Menu); 
                            break;
                        case 3: ChangeState(MenuState::Sniffer_Live); break;
                        case 4: ChangeState(MenuState::Settings_Main); break;
                    }
                    break;
                case MenuState::Recon_AP_List:
                {
                    auto aps = AccessPointManager::GetInstance().GetAccessPoints();
                    if (selectedIndex_ + viewOffset_ < aps.size())
                    {
                        selectedBSSID_ = aps[selectedIndex_ + viewOffset_].bssid;
                        ChangeState(MenuState::Recon_AP_Clients);
                    }
                    break;
                }
                case MenuState::Recon_Station_List:
                {
                    auto allStations = AccessPointManager::GetInstance().GetAllStations();
                    if (selectedIndex_ + viewOffset_ < allStations.size())
                    {
                        auto& st = allStations[selectedIndex_ + viewOffset_];
                        selectedClientMAC_ = st.mac;
                        selectedBSSID_ = st.bssid;
                        isTargetingAP_ = false;
                        previousReconState_ = MenuState::Recon_Station_List;
                        ChangeState(MenuState::Target_Action_Menu);
                    }
                    break;
                }
                case MenuState::Mass_Attacks_Menu:
                    if (selectedIndex_ == 0)
                    {
                        DeauthManager::GetInstance().StartAttack(AttackMode::GlobalSpam);
                        previousReconState_ = MenuState::Mass_Attacks_Menu; 
                        ChangeState(MenuState::Attack_Spam_Menu);
                    }
                    else if (selectedIndex_ == 1)
                    {
                        BeaconSpamManager::GetInstance().Start();
                        previousReconState_ = MenuState::Mass_Attacks_Menu; 
                        ChangeState(MenuState::Attack_Spam_Menu);
                    }
                    break;
                case MenuState::Recon_AP_Clients:
                {
                    auto clients = AccessPointManager::GetInstance().GetStationsForAP(selectedBSSID_);
                    
                    if (selectedIndex_ + viewOffset_ == 0) 
                    {
                        isTargetingAP_ = true;
                        previousReconState_ = MenuState::Recon_AP_Clients;
                        ChangeState(MenuState::Target_Action_Menu);
                    } 
                    else 
                    {
                        size_t clientIndex = selectedIndex_ + viewOffset_ - 1;
                        if (clientIndex < clients.size())
                        {
                            isTargetingAP_ = false;
                            selectedClientMAC_ = clients[clientIndex].mac;
                            previousReconState_ = MenuState::Recon_AP_Clients;
                            ChangeState(MenuState::Target_Action_Menu);
                        }
                    }
                    break;
                }
                case MenuState::Target_Action_Menu:
                {
                    uint8_t targetChannel = 1;
                    auto aps = AccessPointManager::GetInstance().GetAccessPoints();
                    for (const auto& ap : aps)
                    {
                        if (ap.bssid == selectedBSSID_)
                        {
                            targetChannel = ap.channel;
                            break;
                        }
                    }

                    if (selectedIndex_ == 0)
                    {
                        if (isTargetingAP_)
                            DeauthManager::GetInstance().StartAttack(AttackMode::BroadcastAP, selectedBSSID_, targetChannel);
                        else
                            DeauthManager::GetInstance().StartAttack(AttackMode::SingleTarget, selectedBSSID_, targetChannel, selectedClientMAC_);
                        
                        ChangeState(previousReconState_);
                    }
                    else if (selectedIndex_ == 1)
                    {
                        if (isTargetingAP_)
                            DeauthManager::GetInstance().StartAttack(AttackMode::SpamAP, selectedBSSID_, targetChannel);
                        else
                            DeauthManager::GetInstance().StartAttack(AttackMode::SpamTarget, selectedBSSID_, targetChannel, selectedClientMAC_);
                            
                        ChangeState(MenuState::Attack_Spam_Menu);
                    }
                    else if (selectedIndex_ == 2)
                    {
                        BeaconSpamManager::GetInstance().Start(targetChannel);
                        ChangeState(MenuState::Attack_Spam_Menu);
                    }
                    break;
                }
                case MenuState::Attack_Spam_Menu:
                    DeauthManager::GetInstance().StopAttack();
                    ChangeState(previousReconState_);
                    break;

                default: break;
            }
            break;
        }
        case(InputEvent::Back):
        {
            switch (currentState_)
            {
                case MenuState::Recon_AP_Clients:
                    ChangeState(MenuState::Recon_AP_List);
                    break;

                case MenuState::Mass_Attacks_Menu:
                    ChangeState(MenuState::Main);
                    break;

                case MenuState::Target_Action_Menu:
                    ChangeState(previousReconState_);
                    break;

                case MenuState::Attack_Spam_Menu:
                    DeauthManager::GetInstance().StopAttack();
                    BeaconSpamManager::GetInstance().Stop();
                    ChangeState(previousReconState_);
                    break;

                case MenuState::Main:
                    break;

                default:
                    WifiSniffer::GetInstance().Stop();
                    ChangeState(MenuState::Main);
                    break;
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
    
    bool dbTimeToUpdate = (currentTick - lastUiUpdateTick) > pdMS_TO_TICKS(1000); 
    bool attackTimeToUpdate = (currentState_ == MenuState::Attack_Spam_Menu) && ((currentTick - lastUiUpdateTick) > pdMS_TO_TICKS(300));

    bool needDbUpdate = (dataChanged && dbTimeToUpdate);

    if (!cursorMoved && !needDbUpdate && !attackTimeToUpdate) 
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
        
        case MenuState::Target_Action_Menu:
            disp.DrawStatusBar("ВИБІР АТАКИ", 0.85f, "18:25");
            RenderTargetActionMenu();
            break;

        case MenuState::Mass_Attacks_Menu:
            disp.DrawStatusBar("МАСОВІ АТАКИ", 0.85f, "18:25");
            RenderMassAttacksMenu();
            break;

        case MenuState::Attack_Spam_Menu:
            disp.DrawStatusBar("АТАКА...", 0.85f, "18:25");
            RenderAttackScreen();
            break;
            
        default: break;
    }

    lastSelectedIndex_ = selectedIndex_;
    lastViewOffset_ = viewOffset_; 
    lastMenuSize_ = currentMenuSize_;
    
    if (needDbUpdate || attackTimeToUpdate) 
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

void MenuController::RenderTargetActionMenu()
{
    vector<string_view> items = {
        "Один пакет (Deauth)",
        "Спам (Deauth)",
        "Beacon Spam (Канал цілі)"
    };

    currentMenuSize_ = items.size();

    for (size_t i = 0; i < items.size(); ++i)
    {
        DisplayDriver::GetInstance().DrawMenuRow(i, items[i], (i == selectedIndex_));
    }
}

void MenuController::RenderAttackScreen()
{
    auto& disp = DisplayDriver::GetInstance();
    auto& deauth = DeauthManager::GetInstance();
    auto& beaconSpam = BeaconSpamManager::GetInstance();
    
    bool forceFullRedraw = (lastSelectedIndex_ == 255);

    if (beaconSpam.IsActive())
    {
        string targetMacStr = "ГЕНЕРАЦІЯ ФЕЙКІВ (Beacon)";
        uint8_t currentChannel = beaconSpam.GetTargetChannel(); 
        uint32_t packets = beaconSpam.GetPacketsSent();

        disp.DrawAttackTelemetry(targetMacStr, currentChannel, packets, forceFullRedraw);
        return;
    }

    if (deauth.GetCurrentMode() == AttackMode::GlobalSpam)
    {
        string targetMacStr = "ГЛОБАЛЬНИЙ СПАМ (Всі ТД)";
        uint8_t currentChannel = deauth.GetTargetChannel(); 
        uint32_t packets = deauth.GetPacketsSent();

        disp.DrawAttackTelemetry(targetMacStr, currentChannel, packets, forceFullRedraw);
    }
    else 
    {
        uint8_t targetChannel = 1;
        auto aps = AccessPointManager::GetInstance().GetAccessPoints();
        for (const auto& ap : aps)
        {
            if (ap.bssid == selectedBSSID_)
            {
                targetChannel = ap.channel;
                break;
            }
        }

        string targetMacStr;
        if (isTargetingAP_)
            targetMacStr = "Broadcast (Всі клієнти)";
        else
            targetMacStr = selectedClientMAC_.toString();

        uint32_t packets = deauth.GetPacketsSent();

        disp.DrawAttackTelemetry(targetMacStr, targetChannel, packets, forceFullRedraw);
    }
}

void MenuController::RenderMassAttacksMenu()
{
    vector<string_view> items = {
        "Глобальний спам (Deauth)",
        "Beacon Spam (Всі канали)"
    };

    currentMenuSize_ = items.size();

    for (size_t i = 0; i < items.size(); ++i)
    {
        DisplayDriver::GetInstance().DrawMenuRow(i, items[i], (i == selectedIndex_));
    }
}

