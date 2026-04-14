#include "MenuController.hpp"

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
                    case 0: ChangeState(MenuState::Recon_AP_List); break;
                    case 1: ChangeState(MenuState::Attack_Spam_Menu); break;
                    case 2: ChangeState(MenuState::Sniffer_Live); break;
                    case 3: ChangeState(MenuState::Settings_Main); break;
                }
            break;
        }
        case(InputEvent::Back):
        {
            if (currentState_ != MenuState::Main)
                ChangeState(MenuState::Main);
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

    switch (currentState_)
    {
        case MenuState::Main: RenderMainMenu(); break;

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

    DisplayDriver::GetInstance().ClearScreen();
    DisplayDriver::GetInstance().DrawHeader("ГОЛОВНЕ МЕНЮ");

    for (size_t i = 0; i < items.size(); ++i)
    {
        DisplayDriver::GetInstance().DrawMenuRow(i, items[i], (i == selectedIndex_));
    }
}

void MenuController::RenderReconAPList()
{
    vector<string_view> items = {
        "Starlink_5G",
        "Guest_Network",
        "Free_Public_WLAN"
    };

    currentMenuSize_ = items.size();

    DisplayDriver::GetInstance().ClearScreen();
    DisplayDriver::GetInstance().DrawHeader("ЗНАЙДЕНІ МЕРЕЖІ");

    for (size_t i = 0; i < items.size(); ++i)
    {
        int8_t fakeRssi = -40 - (i * 15);
        DisplayDriver::GetInstance().DrawNetworkRow(
            i, 
            items[i], 
            "00:11:22:33:44:55", 
            fakeRssi, 
            (i == selectedIndex_)
        );
    }
}