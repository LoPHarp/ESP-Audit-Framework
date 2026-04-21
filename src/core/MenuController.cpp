#include "MenuController.hpp"
#include "../storage/AccessPointManager.hpp"
#include "../sniffer/wifi_sniffer.hpp"

using namespace std;

MenuController& MenuController::GetInstance()
{
    static MenuController Instance;
    return Instance;
}

MenuController::MenuController() : currentState_(MenuState::Main), selectedIndex_(0), currentMenuSize_(0), viewOffset_(0) {}

void MenuController::Initialize()
{
    DisplayDriver::GetInstance().Initialize();
    InputManager::GetInstance().Initalize();

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

                if(currentState_ == MenuState::Main) RenderMainMenu();
                else if(currentState_ == MenuState::Recon_AP_List) RenderReconAPList();
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

                if(currentState_ == MenuState::Main) RenderMainMenu();
                else if(currentState_ == MenuState::Recon_AP_List) RenderReconAPList();
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
                    case 1: ChangeState(MenuState::Attack_Spam_Menu); break;
                    case 2: ChangeState(MenuState::Sniffer_Live); break;
                    case 3: ChangeState(MenuState::Settings_Main); break;
                }
            }
            break;
        }
        case(InputEvent::Back):
        {
            if (currentState_ != MenuState::Main)
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
    if (currentState_ != newState) 
    {
        selectedIndex_ = 0;
        viewOffset_ = 0;
        DisplayDriver::GetInstance().ClearScreen();
    }

    currentState_ = newState;

    switch (currentState_)
    {
        case MenuState::Main: 
            DisplayDriver::GetInstance().DrawHeader("ГОЛОВНЕ МЕНЮ");
            RenderMainMenu(); 
            break;
        case MenuState::Recon_AP_List:
            DisplayDriver::GetInstance().DrawHeader("ЗНАЙДЕНІ МЕРЕЖІ");
            RenderReconAPList(); 
            break;
        default: break;
    }
}

void MenuController::RenderMainMenu()
{
    vector<string_view> items = {
        "Розвідка (Scan)",
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
    APVVector items = AccessPointManager::GetInstance().GetNetworks();
    currentMenuSize_ = items.size();

    if (items.empty()) return;

    size_t displayCount = min<size_t>(currentMenuSize_ - viewOffset_, 8);

    for (size_t i = 0; i < displayCount; ++i)
    {
        size_t itemIndex = viewOffset_ + i;
        bool isSelected = (itemIndex == selectedIndex_);
        
        visit([i, isSelected](const auto& AP) {
            using T = decay_t<decltype(AP)>;
            
            if constexpr (is_same_v<T, BeaconFrame>)
            {
                DisplayDriver::GetInstance().DrawNetworkRow(i, AP.ssid, AP.base.source.toString(), AP.base.rssi, isSelected);
            }
            else if constexpr (is_same_v<T, ProbeRequestFrame>)
            {
                DisplayDriver::GetInstance().DrawNetworkRow(i, AP.ssid, AP.base.source.toString(), AP.base.rssi, isSelected);
            }
        }, items[itemIndex]);
    }
}

void MenuController::Update()
{
    if(currentState_ != MenuState::Recon_AP_List) return;

    static uint32_t lastDataVersion = 0;
    static uint32_t lastDrawTick = 0;

    uint32_t currentVersion = AccessPointManager::GetInstance().GetDataVersion();
    uint32_t currentTick = xTaskGetTickCount();

    if (currentVersion != lastDataVersion)
    {
        if (lastDrawTick == 0 || (currentTick - lastDrawTick > pdMS_TO_TICKS(1500)))
        {
            lastDataVersion = currentVersion;
            lastDrawTick = currentTick;
            RenderReconAPList();
        }
    }
    else if (currentMenuSize_ == 0)
    {
        static uint32_t lastDotTick = 0;
        if(currentTick - lastDotTick > pdMS_TO_TICKS(500))
        {
            lastDotTick = currentTick;
            static uint8_t dotCount = 0;
            dotCount = (dotCount + 1) % 4;
            DisplayDriver::GetInstance().DrawSearchingAnimation(dotCount);
        }
    }
}