#pragma once

#include "IOutputTarget.hpp"

#include <vector>
#include <memory>
#include <string_view>

class OutputManager
{
public:
    void AddTarget(std::unique_ptr<IOutputTarget> target);
    void InitializeAll();
    void Broadcast(std::string_view data);
private:
    std::vector<std::unique_ptr<IOutputTarget>> targets_;
};