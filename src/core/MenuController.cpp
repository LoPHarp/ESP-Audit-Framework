#include "MenuController.hpp"
#include "../storage/AccessPointManager.hpp"
#include "../sniffer/wifi_sniffer.hpp"

using namespace std;

MenuController& MenuController::GetInstance()
{
    static MenuController Instance;
    return Instance;
}

MenuController::MenuController() : currentState_(MenuState::Main), selectedIndex_(0), currentMenuSize_(0) {}

void MenuController::Initialize()
{
    DisplayDriver::GetInstance().Initialize();
    InputManager::GetInstance().Initalize();

    ChangeState(MenuState::Main);
    GetInstance();
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
                if(selectedIndex_ == 0)
                    selectedIndex_ = currentMenuSize_ - 1;
                else
                    selectedIndex_--;
                ChangeState(currentState_);
            }
            break;
        }
        case(InputEvent::Down):
        {
            if(currentMenuSize_ > 0)
            {
                selectedIndex_ = (selectedIndex_ + 1) % currentMenuSize_;
                ChangeState(currentState_);
            }
            break;
        }
        case(InputEvent::Select):
        {
            if(currentState_ == MenuState::Main)
                switch (selectedIndex_)
                {
                    case 0:
                    {
                        WifiSniffer::GetInstance().Start();
                        ChangeState(MenuState::Recon_AP_List); 
                        break;
                    }
                    case 1: ChangeState(MenuState::Attack_Spam_Menu); break;
                    case 2: ChangeState(MenuState::Sniffer_Live); break;
                    case 3: ChangeState(MenuState::Settings_Main); break;
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
        default:
            break;
    }
}

void MenuController::ChangeState(MenuState newState)
{
    if (currentState_ != newState) selectedIndex_ = 0;

    currentState_ = newState;
    DisplayDriver::GetInstance().ClearScreen();

    switch (currentState_)
    {
        case MenuState::Main:
        {
            DisplayDriver::GetInstance().DrawHeader("ГОЛОВНЕ МЕНЮ"); 
            RenderMainMenu(); 
            break;
        }
        case MenuState::Recon_AP_List: 
        {
            DisplayDriver::GetInstance().DrawHeader("ЗНАЙДЕНІ МЕРЕЖІ");
            RenderReconAPList(); 
            break;
        }

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

    size_t displayLimit = min<size_t>(items.size(), 10);

    for (size_t i = 0; i < displayLimit; ++i)
    {
        bool isSelected = (i == selectedIndex_);
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
        }, items[i]);
    }
}

void MenuController::Update()
{
    if(currentState_ != MenuState::Recon_AP_List)
        return;

    if(currentMenuSize_ != 0)
        return;

    static uint32_t lastTick = 0;
    static uint8_t dotCount = 0;

    uint32_t currentTick = xTaskGetTickCount();
    
    if(currentTick - lastTick > pdMS_TO_TICKS(500))
    {
        lastTick = currentTick;
        
        APVVector items = AccessPointManager::GetInstance().GetNetworks();
        
        if (items.empty())
        {
            static uint8_t dotCount = 0;
            dotCount = (dotCount + 1) % 4;
            DisplayDriver::GetInstance().DrawSearchingAnimation(dotCount);
        }
        else
        {
            RenderReconAPList();
        }
    }
}