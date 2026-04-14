#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include "../input/InputManager.hpp"
#include "../output/DisplayDriver.hpp"

enum class MenuState 
{
    // --- 1. Головне меню ---
    Main,                   // Головне меню екрану

    // --- 2. Розвідка (Reconnaissance) ---
    Recon_AP_List,          // Список знайдених точок доступу (Wi-Fi роутерів)
    Recon_Station_List,     // Список знайдених клієнтів (телефонів, смарт-годинників)

    // --- 3. Цільові атаки (Targeted Attacks) ---
    Target_Action_Menu,     // Меню вибору атаки на конкретну ціль (наприклад, Deauth)

    // --- 4. Масові атаки (Broadcast/Spam) ---
    Attack_Spam_Menu,       // Меню масових атак (Beacon Spam, Global Deauth)

    // --- 5. Пасивний сніфер (Sniffer Mode) ---
    Sniffer_Live,           // Live-термінал перехоплених пакетів у реальному часі

    // --- 6. Налаштування (Settings) ---
    Settings_Main,          // Головне меню налаштувань
    Settings_MacSpoofing,   // Налаштування підміни MAC-адреси
    Settings_Storage        // Налаштування збереження (SD-карта або USB-вивантаження)
};

class MenuController
{
public:
    static MenuController& GetInstance();

    void Initialize();
    void ProcessInput();

private:
    MenuController();
    ~MenuController() = default;

    MenuController(const MenuController&) = delete;
    MenuController& operator=(const MenuController&) = delete;

    MenuState currentState_;      
    uint8_t selectedIndex_;       
    uint8_t currentMenuSize_;     

    void ChangeState(MenuState newState); 
    void RenderMainMenu();

    void RenderReconAPList();
};