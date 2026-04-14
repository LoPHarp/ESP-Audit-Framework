#include "core/core_application.hpp"

extern "C" void app_main()
{
    CoreApplication app;
    app.Initialize();
    app.Run();
}