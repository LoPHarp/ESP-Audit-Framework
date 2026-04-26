#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include "../input/InputManager.hpp"
#include "../output/DisplayDriver.hpp"
#include "../storage/AccessPointManager.hpp"

enum class MenuState 
{
    Main,
    Recon_AP_List,
    Recon_AP_Clients,      
    Recon_Station_List,
    Target_Action_Menu,
    Attack_Spam_Menu,
    Sniffer_Live,
    Settings_Main,
    Settings_MacSpoofing,
    Settings_Storage 
};

class MenuController
{
public:
    static MenuController& GetInstance();

    void Initialize();
    void ProcessInput();
    void Update();

private:
    MenuController() = default;
    ~MenuController() = default;

    MenuController(const MenuController&) = delete;
    MenuController& operator=(const MenuController&) = delete;

    MenuState currentState_;      
    uint8_t selectedIndex_;       
    uint8_t lastSelectedIndex_;   
    uint8_t currentMenuSize_;     
    size_t viewOffset_ = 0;
    
    size_t lastViewOffset_ = 0;
    uint8_t lastMenuSize_ = 0;

    uint32_t lastDataVersion_ = 0;
    MacAddress selectedBSSID_;   

    void ChangeState(MenuState newState); 
    
    void RenderMainMenu();
    void RenderReconAPList();
    void RenderReconAPClients();  
    void RenderReconStationList();
};