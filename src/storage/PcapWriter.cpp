#include "PcapWriter.hpp"
#include "../attacks/HandshakeCatcher.hpp"

#include <cstdio>
#include <esp_timer.h>
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include "driver/gpio.h"
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <iostream>

using namespace std;

PcapWriter& PcapWriter::GetInstance()
{
    static PcapWriter instance;
    return instance;
}

void PcapWriter::Initialize()
{
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = GPIO_NUM_13;
    bus_cfg.miso_io_num = GPIO_NUM_12;
    bus_cfg.sclk_io_num = GPIO_NUM_14;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        cout << "[SPI] Помилка ініціалізації HSPI: " << esp_err_to_name(ret) << endl;
        isCardMounted_ = false;
        return;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {}; 
    mount_config.format_if_mount_failed = true;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;
    mount_config.disk_status_check_enable = false;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST; 
    host.max_freq_khz = 400;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_5;
    slot_config.host_id = SPI2_HOST;

    sdmmc_card_t* card;
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) 
    {
        cout << "[SD CARD] Помилка монтування! Код: " << esp_err_to_name(ret) << endl;
        isCardMounted_ = false;
        return;
    }

    cout << "[SD CARD] Успішно змонтовано!" << endl;
    isCardMounted_ = true;
    StartLogging();
}

void PcapWriter::StartLogging()
{
    if (isLogging_) return;
    if (!isCardMounted_)
    {
        cout << "[PCAP] Помилка: SD карта не підключена!" << endl;
        return;
    }

    isLogging_ = true;
    WriteGlobalHeaderIfNeeded(currentFilePath_);
    
    xTaskCreate(WriterTask, "PcapWriterTask", 4096, this, 1, &writerTaskHandle_);
}

void PcapWriter::StopLogging()
{
    if (!isLogging_) return;
    isLogging_ = false;

    if (writerTaskHandle_ != nullptr)
    {
        vTaskDelete(writerTaskHandle_);
        writerTaskHandle_ = nullptr;
    }
    
    FlushBufferToFile();
}

void PcapWriter::WriteGlobalHeaderIfNeeded(const string& filePath)
{
    FILE* f = fopen(filePath.c_str(), "r");
    if (f != nullptr)
    {
        fclose(f);
        return;
    }

    f = fopen(filePath.c_str(), "w");
    if (f != nullptr)
    {
        PcapGlobalHeader globalHdr;
        globalHdr.magic_number = 0xa1b2c3d4;
        globalHdr.version_major = 2;
        globalHdr.version_minor = 4;
        globalHdr.thiszone = 0;
        globalHdr.sigfigs = 0;
        globalHdr.snaplen = 65535;
        globalHdr.network = 105;

        fwrite(&globalHdr, sizeof(PcapGlobalHeader), 1, f);
        fclose(f);
    }
}

void PcapWriter::FlushBufferToFile()
{
    auto packets = HandshakeCatcher::GetInstance().GetAndClearBuffer();
    if (packets.empty()) return;

    FILE* f = fopen(currentFilePath_.c_str(), "a");
    if (f == nullptr) return; 

    static char fileBuf[8192]; 
    setvbuf(f, fileBuf, _IOFBF, sizeof(fileBuf));

    for (const auto& packet : packets)
    {
        PcapPacketHeader pktHdr;
        uint64_t timeUs = esp_timer_get_time();
        
        pktHdr.ts_sec = timeUs / 1000000;
        pktHdr.ts_usec = timeUs % 1000000;
        pktHdr.incl_len = packet.size();
        pktHdr.orig_len = packet.size();

        fwrite(&pktHdr, sizeof(PcapPacketHeader), 1, f);
        fwrite(packet.data(), 1, packet.size(), f);
    }

    fclose(f); 
}

void PcapWriter::WriterTask(void* arg)
{
    PcapWriter* writer = static_cast<PcapWriter*>(arg);
    uint32_t msCounter = 0;

    while (writer->isLogging_)
    {       
        size_t bufferedBytes = HandshakeCatcher::GetInstance().GetBufferedBytesCount();
        
        if (bufferedBytes > 0 || msCounter >= 2000) 
        {
            if (bufferedBytes > 0) {
                writer->FlushBufferToFile();
            }
            msCounter = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
        msCounter += 100;
    }
    
    writer->FlushBufferToFile();
    vTaskDelete(nullptr);
}