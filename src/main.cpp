#include "core/core_application.h"
#include <memory>

using namespace std;

extern "C" void app_main()
{
    auto app = make_unique<CoreApplication>();
    
    app->Initialize();
    app->Run();
}