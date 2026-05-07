#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#pragma pack(push, 1)
struct PcapGlobalHeader 
{
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;   
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
};

struct PcapPacketHeader 
{
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};
#pragma pack(pop)

class PcapWriter
{
public:
    static PcapWriter& GetInstance();

    void Initialize();
    void StartLogging();
    void StopLogging();

private:
    PcapWriter() = default;
    ~PcapWriter() = default;
    PcapWriter(const PcapWriter&) = delete;
    PcapWriter& operator=(const PcapWriter&) = delete;

    static void WriterTask(void* arg);
    void WriteGlobalHeaderIfNeeded(const std::string& filePath);
    void FlushBufferToFile();

    TaskHandle_t writerTaskHandle_{nullptr};
    bool isLogging_{false};
    std::string currentFilePath_{"/sdcard/handshakes.pcap"};
};