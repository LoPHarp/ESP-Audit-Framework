#pragma once
#include <string_view>

class IOutputTarget
{
public:
    virtual ~IOutputTarget() = default;
    virtual bool Initialize() = 0;
    virtual bool WriteLine(std::string_view data) = 0;
};
