#include "core_application.h"

using namespace std;

void CoreApplication::Initialize() 
{
}

void CoreApplication::Run()
{
    for (int i = 0; i < 5; ++i)
        Initialize();
}