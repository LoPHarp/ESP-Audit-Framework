#pragma once 

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class InputEvent
{
    None,
    Up,
    Down,
    Select,
    Back
};

class InputManager
{
public:
    static InputManager& GetInstance();

    void Initalize();
    InputEvent GetEvent(TickType_t wait_ticks = 0);
private:
    InputManager();
    ~InputManager() = default;

    InputManager(const InputManager&) = delete;
    InputManager operator=(const InputManager&) = delete;

    QueueHandle_t event_queue_;

    static void InputTask(void* arg);
};