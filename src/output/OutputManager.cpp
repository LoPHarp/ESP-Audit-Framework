#include "OutputManager.h"
#include <utility>

using namespace std;

void OutputManager::AddTarget(unique_ptr<IOutputTarget> target)
{
    if (target)
        targets_.push_back(move(target));
}

void OutputManager::InitializeAll()
{
    for (auto& target : targets_)
        target->Initialize();
}

void OutputManager::Broadcast(string_view data)
{
    for (auto& target : targets_)
        target->WriteLine(data);
}