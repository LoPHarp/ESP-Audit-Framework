#include "MenuController.hpp"
#include "../sniffer/wifi_sniffer.hpp"
#include "../attacks/DeauthManager.hpp"
#include "../attacks/BeaconSpamManager.hpp"
#include "../attacks/PmkidManager.hpp"
#include "../attacks/HandshakeCatcher.hpp"

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
                uint8_t minIndex = (currentState_ == MenuState::Target_Action_Menu) ? 2 : 0;
                if(selectedIndex_ > minIndex) selectedIndex_--;
                else selectedIndex_ = currentMenuSize_ - 1;

                if (selectedIndex_ < viewOffset_) viewOffset_ = selectedIndex_;
                else if (selectedIndex_ == currentMenuSize_ - 1) viewOffset_ = (currentMenuSize_ > 8) ? currentMenuSize_ - 8 : 0;
            }
            break;
        }
        case(InputEvent::Down):
        {
            if(currentMenuSize_ > 0)
            {
                uint8_t minIndex = (currentState_ == MenuState::Target_Action_Menu) ? 2 : 0;
                selectedIndex_++;
                if (selectedIndex_ >= currentMenuSize_) selectedIndex_ = minIndex;

                if (selectedIndex_ >= viewOffset_ + 8) viewOffset_ = selectedIndex_ - 7;
                else if (selectedIndex_ == minIndex) viewOffset_ = 0;
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
                        case 0: WifiSniffer::GetInstance().Start(); ChangeState(MenuState::Recon_AP_List); break;
                        case 1: WifiSniffer::GetInstance().Start(); ChangeState(MenuState::Recon_Station_List); break;
                        case 2: WifiSniffer::GetInstance().Start(); ChangeState(MenuState::Mass_Attacks_Menu); break;
                        case 3: ChangeState(MenuState::Settings_Main); break;
                    }
                    break;
                case MenuState::Recon_AP_List:
                {
                    auto aps = AccessPointManager::GetInstance().GetAccessPoints();
                    if (selectedIndex_ < aps.size()) 
                    {
                        selectedBSSID_ = aps[selectedIndex_].bssid;
                        isTargetingAP_ = true;
                        sourceListState_ = MenuState::Recon_AP_List; 
                        ChangeState(MenuState::Target_Action_Menu);
                        selectedIndex_ = 2;
                    }
                    break;
                }
                case MenuState::Recon_Station_List:
                {
                    auto allStations = AccessPointManager::GetInstance().GetAllStations();
                    if (selectedIndex_ < allStations.size()) 
                    {
                        auto& st = allStations[selectedIndex_];
                        selectedClientMAC_ = st.mac;
                        selectedBSSID_ = st.bssid;
                        isTargetingAP_ = false;
                        clientReturnState_ = MenuState::Recon_Station_List; 
                        ChangeState(MenuState::Target_Action_Menu);
                        selectedIndex_ = 2;
                    }
                    break;
                }
                case MenuState::Mass_Attacks_Menu:
                    if (selectedIndex_ == 0) 
                    {
                        DeauthManager::GetInstance().StartAttack(AttackMode::GlobalSpam);
                        attackReturnState_ = MenuState::Mass_Attacks_Menu; 
                        ChangeState(MenuState::Attack_Spam_Menu);
                    } 
                    else if (selectedIndex_ == 1) 
                    {
                        BeaconSpamManager::GetInstance().Start();
                        attackReturnState_ = MenuState::Mass_Attacks_Menu; 
                        ChangeState(MenuState::Attack_Spam_Menu);
                    }
                    break;
                case MenuState::Recon_AP_Clients:
                {
                    auto clients = AccessPointManager::GetInstance().GetStationsForAP(selectedBSSID_);
                    if (selectedIndex_ < clients.size()) 
                    {
                        isTargetingAP_ = false;
                        selectedClientMAC_ = clients[selectedIndex_].mac;
                        clientReturnState_ = MenuState::Recon_AP_Clients; 
                        ChangeState(MenuState::Target_Action_Menu);
                        selectedIndex_ = 2;
                    }
                    break;
                }
                case MenuState::Target_Action_Menu:
                {
                    uint8_t targetChannel = 1;
                    string targetSsid = "";
                    auto aps = AccessPointManager::GetInstance().GetAccessPoints();
                    for (const auto& ap : aps) 
                    {
                        if (ap.bssid == selectedBSSID_) 
                        {
                            targetChannel = ap.channel;
                            targetSsid = string(ap.ssid);
                            break;
                        }
                    }

                    if (isTargetingAP_)
                    {
                        if (selectedIndex_ == 2) 
                        {
                            ChangeState(MenuState::Recon_AP_Clients);
                        } 
                        else if (selectedIndex_ == 3) 
                        {
                            DeauthManager::GetInstance().StartAttack(AttackMode::BroadcastAP, selectedBSSID_, targetChannel);
                        } 
                        else if (selectedIndex_ == 4) 
                        {
                            DeauthManager::GetInstance().StartAttack(AttackMode::SpamAP, selectedBSSID_, targetChannel);
                            attackReturnState_ = MenuState::Target_Action_Menu;
                            ChangeState(MenuState::Attack_Spam_Menu);
                        } 
                        else if (selectedIndex_ == 5) 
                        {
                            BeaconSpamManager::GetInstance().Start(targetChannel);
                            attackReturnState_ = MenuState::Target_Action_Menu;
                            ChangeState(MenuState::Attack_Spam_Menu);
                        } 
                        else if (selectedIndex_ == 6) 
                        {
                            PmkidManager::GetInstance().StartAttack(selectedBSSID_, targetSsid, targetChannel);
                            attackReturnState_ = MenuState::Target_Action_Menu;
                            ChangeState(MenuState::Attack_Spam_Menu);
                        }
                    }
                    else
                    {
                        if (selectedIndex_ == 2) 
                        {
                            DeauthManager::GetInstance().StartAttack(AttackMode::SingleTarget, selectedBSSID_, targetChannel, selectedClientMAC_);
                        } 
                        else if (selectedIndex_ == 3) 
                        {
                            DeauthManager::GetInstance().StartAttack(AttackMode::SpamTarget, selectedBSSID_, targetChannel, selectedClientMAC_);
                            attackReturnState_ = MenuState::Target_Action_Menu;
                            ChangeState(MenuState::Attack_Spam_Menu);
                        } 
                        else if (selectedIndex_ == 4) 
                        {
                            BeaconSpamManager::GetInstance().Start(targetChannel);
                            attackReturnState_ = MenuState::Target_Action_Menu;
                            ChangeState(MenuState::Attack_Spam_Menu);
                        }
                    }
                    break;
                }
                case MenuState::Attack_Spam_Menu:
                    DeauthManager::GetInstance().StopAttack();
                    BeaconSpamManager::GetInstance().Stop();
                    PmkidManager::GetInstance().StopAttack();
                    ChangeState(attackReturnState_);
                    if (attackReturnState_ == MenuState::Target_Action_Menu)
                        selectedIndex_ = 2; 
                    break;
                case MenuState::Settings_Main:
                    if (selectedIndex_ == 0)
                    {
                        bool current = HandshakeCatcher::GetInstance().IsPassiveCollectionEnabled();
                        HandshakeCatcher::GetInstance().SetPassiveCollection(!current);
                        lastSelectedIndex_ = 255;
                    }
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
                    isTargetingAP_ = true;
                    ChangeState(MenuState::Target_Action_Menu);
                    selectedIndex_ = 2;
                    break;
                case MenuState::Target_Action_Menu:
                    if (isTargetingAP_) 
                    {
                        ChangeState(sourceListState_);
                    } 
                    else 
                    {
                        ChangeState(clientReturnState_);
                    }
                    break;
                case MenuState::Mass_Attacks_Menu:
                    ChangeState(MenuState::Main);
                    break;
                case MenuState::Attack_Spam_Menu:
                    DeauthManager::GetInstance().StopAttack();
                    BeaconSpamManager::GetInstance().Stop();
                    PmkidManager::GetInstance().StopAttack();
                    ChangeState(attackReturnState_);
                    break;
                case MenuState::Main:
                    break;
                case MenuState::Settings_Main:
                    ChangeState(MenuState::Main);
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
    
    bool dbTimeToUpdate = (currentTick - lastUiUpdateTick) > pdMS_TO_TICKS(2000); 
    bool attackTimeToUpdate = (currentState_ == MenuState::Attack_Spam_Menu) && ((currentTick - lastUiUpdateTick) > pdMS_TO_TICKS(300));

    bool needDbUpdate = (dataChanged && dbTimeToUpdate);

    if (!cursorMoved && !needDbUpdate && !attackTimeToUpdate) 
        return;

    string statusBarTitle = "МАРАУДЕР";
    
    if (currentState_ == MenuState::Main) statusBarTitle = "ГОЛОВНЕ МЕНЮ";
    else if (currentState_ == MenuState::Recon_AP_List) statusBarTitle = "ТОЧКИ ДОСТУПУ";
    else if (currentState_ == MenuState::Recon_Station_List) statusBarTitle = "ГЛОБАЛЬНИЙ ПОШУК";
    else if (currentState_ == MenuState::Mass_Attacks_Menu) statusBarTitle = "МАСОВІ АТАКИ";
    else if (currentState_ == MenuState::Recon_AP_Clients || currentState_ == MenuState::Target_Action_Menu || currentState_ == MenuState::Attack_Spam_Menu) 
    {
        bool isGlobalAttack = false;
        if (currentState_ == MenuState::Attack_Spam_Menu) 
        {
            if (DeauthManager::GetInstance().GetCurrentMode() == AttackMode::GlobalSpam ||
               (BeaconSpamManager::GetInstance().IsActive() && BeaconSpamManager::GetInstance().GetTargetChannel() == BEACON_SPAM_HOPPING)) {
                isGlobalAttack = true;
                statusBarTitle = "МАСОВА АТАКА";
            }
        }
        
        if (!isGlobalAttack) 
        {
            auto aps = db.GetAccessPoints();
            for (const auto& ap : aps) {
                if (ap.bssid == selectedBSSID_) {
                    statusBarTitle = string(ap.ssid);
                    if (statusBarTitle.empty()) statusBarTitle = "<Прихована>";
                    break;
                }
            }
        }
    }

    uint32_t sessionEapol = HandshakeCatcher::GetInstance().GetSessionEapolCount();
    disp.DrawStatusBar(statusBarTitle, 0.85f, "18:25", sessionEapol);

    switch (currentState_)
    {
        case MenuState::Main: RenderMainMenu(); break;
        case MenuState::Recon_AP_List: RenderReconAPList(); break;
        case MenuState::Recon_AP_Clients: RenderReconAPClients(); break;
        case MenuState::Recon_Station_List: RenderReconStationList(); break;
        case MenuState::Target_Action_Menu: RenderTargetActionMenu(); break;
        case MenuState::Mass_Attacks_Menu: RenderMassAttacksMenu(); break;
        case MenuState::Attack_Spam_Menu: RenderAttackScreen(); break;
        case MenuState::Settings_Main: RenderSettingsMain(); break;
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

    currentMenuSize_ = clients.size();

    for (uint8_t i = 0; i < 8 && (i + viewOffset_) < currentMenuSize_; ++i)
    {
        size_t actualIndex = i + viewOffset_;
        bool isSelected = (actualIndex == selectedIndex_);
        
        bool cursorMoved = (selectedIndex_ != lastSelectedIndex_);
        bool viewMoved = (viewOffset_ != lastViewOffset_);
        bool isNewRow = (actualIndex >= lastMenuSize_);

        bool force = (lastSelectedIndex_ == 255) || viewMoved || isNewRow || 
                     (isSelected && cursorMoved) || (actualIndex == lastSelectedIndex_ && cursorMoved);

        auto& st = clients[actualIndex];
        disp.DrawAPClientRow(i, st.mac.toString(), st.lastSeen, st.rssi, isSelected, force);
    }

    if (clients.empty())
        disp.DrawSearchingAnimation( (pdTICKS_TO_MS(xTaskGetTickCount()) / 1000) % 4, 0 );
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
    auto& disp = DisplayDriver::GetInstance();
    auto aps = AccessPointManager::GetInstance().GetAccessPoints();
    
    AccessPoint currentAP;
    for (const auto& ap : aps)
    {
        if (ap.bssid == selectedBSSID_)
        {
            currentAP = ap;
            break;
        }
    }

    if (isTargetingAP_)
    {
        string macStr = "MAC: " + currentAP.bssid.toString();
        
        string pmfStr = currentAP.security.isPMFRequired ? "REQ" : (currentAP.security.isPMFCapable ? "CAP" : "NO");
        string wpa3Str = currentAP.security.isWPA3 ? "ТАК" : "НІ";
        string secStr = "[ WPA3: " + wpa3Str + " | PMF: " + pmfStr + " ]";

        auto clients = AccessPointManager::GetInstance().GetStationsForAP(selectedBSSID_);
        string clientsStr = "Показати клієнтів (" + to_string(clients.size()) + ")";

        bool deauthBlocked = currentAP.security.isPMFRequired || currentAP.security.isWPA3;
        bool pmkidBlocked = currentAP.security.isWPA3;

        currentMenuSize_ = 7;

        disp.DrawActionRow(0, macStr, false, TFT_CYAN, true);
        disp.DrawActionRow(1, secStr, false, TFT_CYAN, true);
        disp.DrawActionRow(2, "> " + clientsStr, selectedIndex_ == 2, TFT_WHITE, false);
        disp.DrawActionRow(3, "> Один пакет (Deauth)", selectedIndex_ == 3, deauthBlocked ? TFT_RED : TFT_WHITE, false);
        disp.DrawActionRow(4, "> Спам (Deauth)", selectedIndex_ == 4, deauthBlocked ? TFT_RED : TFT_WHITE, false);
        disp.DrawActionRow(5, "> Beacon Spam (Канал)", selectedIndex_ == 5, TFT_WHITE, false);
        disp.DrawActionRow(6, "> Атака PMKID", selectedIndex_ == 6, pmkidBlocked ? TFT_RED : TFT_WHITE, false);
    }
    else
    {
        string macStr = "КЛІЄНТ: " + selectedClientMAC_.toString();
        string apStr = "МЕРЕЖА: " + string(currentAP.ssid).substr(0, 18);
        
        bool deauthBlocked = currentAP.security.isPMFRequired;

        currentMenuSize_ = 5;

        disp.DrawActionRow(0, macStr, false, TFT_CYAN, true);
        disp.DrawActionRow(1, apStr, false, TFT_CYAN, true);
        disp.DrawActionRow(2, "> Один пакет (Deauth)", selectedIndex_ == 2, deauthBlocked ? TFT_RED : TFT_WHITE, false);
        disp.DrawActionRow(3, "> Спам (Deauth)", selectedIndex_ == 3, deauthBlocked ? TFT_RED : TFT_WHITE, false);
        disp.DrawActionRow(4, "> Beacon Spam (Канал)", selectedIndex_ == 4, TFT_WHITE, false);
    }
}

void MenuController::RenderAttackScreen()
{
    auto& disp = DisplayDriver::GetInstance();
    auto& deauth = DeauthManager::GetInstance();
    auto& beaconSpam = BeaconSpamManager::GetInstance();
    auto& pmkid = PmkidManager::GetInstance();

    bool forceFullRedraw = (lastSelectedIndex_ == 255);

    uint32_t eapol = 0;
    if (deauth.GetCurrentMode() == AttackMode::GlobalSpam || (beaconSpam.IsActive() && beaconSpam.GetTargetChannel() == BEACON_SPAM_HOPPING))
    {
        eapol = HandshakeCatcher::GetInstance().GetPacketsInQueue();
    }
    else
    {
        eapol = HandshakeCatcher::GetInstance().GetTargetEapolCount();
    }

    if (beaconSpam.IsActive())
    {
        string targetMacStr = "ГЕНЕРАЦІЯ ФЕЙКІВ (Beacon)";
        uint8_t currentChannel = beaconSpam.GetTargetChannel(); 
        uint32_t packets = beaconSpam.GetPacketsSent();

        disp.DrawAttackTelemetry(targetMacStr, currentChannel, packets, eapol, forceFullRedraw);
        if (eapol > 0) lastDataVersion_++; 
        return;
    }

    if (pmkid.IsActive())
    {
        string targetMacStr = pmkid.GetTargetAp().toString();
        uint32_t auth = pmkid.GetAuthSent();
        uint32_t assoc = pmkid.GetAssocSent();
        bool caught = pmkid.IsHandshakeCaught();
        
        disp.DrawPmkidTelemetry(targetMacStr, auth, assoc, caught, forceFullRedraw);
        
        if (caught) lastDataVersion_++; 
        return;
    }

    if (deauth.GetCurrentMode() == AttackMode::GlobalSpam)
    {
        string targetMacStr = "ГЛОБАЛЬНИЙ СПАМ (Всі ТД)";
        uint8_t currentChannel = deauth.GetTargetChannel(); 
        uint32_t packets = deauth.GetPacketsSent();

        disp.DrawAttackTelemetry(targetMacStr, currentChannel, packets, eapol, forceFullRedraw);
        if (eapol > 0) lastDataVersion_++;
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

        disp.DrawAttackTelemetry(targetMacStr, targetChannel, packets, eapol, forceFullRedraw);
        if (eapol > 0) lastDataVersion_++;
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

void MenuController::RenderSettingsMain()
{
    bool isPassive = HandshakeCatcher::GetInstance().IsPassiveCollectionEnabled();
    currentMenuSize_ = 1;
    DisplayDriver::GetInstance().DrawSettingRow(0, "Пасив. збір EAPOL", isPassive, (selectedIndex_ == 0));
}
