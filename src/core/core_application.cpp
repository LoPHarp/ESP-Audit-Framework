#include "core_application.h"
#include "../sniffer/wifi_sniffer.h"

#include <iostream>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>

using namespace std;

void CoreApplication::Initialize() 
{
    cout << "Initializing hardware and system services..." << endl;

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

    cout << "System initialization completed successfully!" << endl;
}

void CoreApplication::Run()
{
    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}