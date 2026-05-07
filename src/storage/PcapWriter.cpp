#include "PcapWriter.hpp"
#include "../attacks/HandshakeCatcher.hpp"

#include <cstdio>
#include <esp_timer.h>
#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>

using namespace std;

PcapWriter& PcapWriter::GetInstance()
{
    static PcapWriter instance;
    return instance;
}

void PcapWriter::Initialize()
{
    // Здесь будет код инициализации SPI для SD карты
    // Обычно это esp_vfs_fat_sdspi_mount()
    // Пока оставим заглушку, чтобы сфокусироваться на логике PCAP
}

void PcapWriter::StartLogging()
{
    if (isLogging_) return;
    
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
        
        if (bufferedBytes > 65536 || msCounter >= 5000) 
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