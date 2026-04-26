#include "InputManager.hpp"

using namespace std;

static constexpr gpio_num_t PIN_UP = GPIO_NUM_32;
static constexpr gpio_num_t PIN_DOWN = GPIO_NUM_33;
static constexpr gpio_num_t PIN_SEL = GPIO_NUM_25;
static constexpr gpio_num_t PIN_BACK = GPIO_NUM_26;

InputManager& InputManager::GetInstance()
{
    static InputManager instance;
    return instance;
}

InputManager::InputManager() : event_queue_(nullptr) {}

void InputManager::Initialize()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pin_bit_mask = (1ULL << PIN_UP) | (1ULL << PIN_DOWN) |  (1ULL << PIN_SEL) | (1ULL << PIN_BACK);

    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    gpio_config(&io_conf);

    event_queue_ = xQueueCreate(10, sizeof(InputEvent));
    xTaskCreate(InputTask, "InputTask", 2048, this, 5, nullptr);
}

InputEvent InputManager::GetEvent(TickType_t wait_ticks)
{
    InputEvent event = InputEvent::None;
    if (event_queue_ != nullptr)
    {
        xQueueReceive(event_queue_, &event, wait_ticks);
    }
    return event;
}

void InputManager::InputTask(void* arg)
{
    InputManager* manager = static_cast<InputManager*>(arg);
    InputEvent last_event = InputEvent::None;

    while (true)
    {
        InputEvent current_event = InputEvent::None;

        if (gpio_get_level(PIN_UP) == 0) current_event = InputEvent::Up;
        else if (gpio_get_level(PIN_DOWN) == 0) current_event = InputEvent::Down;
        else if (gpio_get_level(PIN_SEL) == 0) current_event = InputEvent::Select;
        else if (gpio_get_level(PIN_BACK) == 0) current_event = InputEvent::Back;

        if (current_event != InputEvent::None && current_event != last_event)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            
            bool still_pressed = false;
            if (current_event == InputEvent::Up && gpio_get_level(PIN_UP) == 0) still_pressed = true;
            else if (current_event == InputEvent::Down && gpio_get_level(PIN_DOWN) == 0) still_pressed = true;
            else if (current_event == InputEvent::Select && gpio_get_level(PIN_SEL) == 0) still_pressed = true;
            else if (current_event == InputEvent::Back && gpio_get_level(PIN_BACK) == 0) still_pressed = true;

            if (still_pressed)
            {
                xQueueSend(manager->event_queue_, &current_event, 0);
            }
        }

        last_event = current_event;
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}