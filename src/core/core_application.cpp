#include "core_application.hpp"
#include "MenuController.hpp"
#include "../tests/test_manager.hpp"

#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace std;

void CoreApplication::Initialize() 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    DisplayDriver::GetInstance().Initialize();
    AccessPointManager::GetInstance().Initialize();
    InputManager::GetInstance().Initialize(); 
    MenuController::GetInstance().Initialize();
}

void CoreApplication::Run()
{
    while (true)
    {
        MenuController::GetInstance().ProcessInput();
        MenuController::GetInstance().Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}