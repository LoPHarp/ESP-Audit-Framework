#include "core/core_application.h"
#include "tests/test_manager.h"
#include <memory>

using namespace std;

extern "C" void app_main()
{
    auto app = make_unique<CoreApplication>();
    
    TestManager::runAllTests();

    app->Initialize();
    app->Run();
}