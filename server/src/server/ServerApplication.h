#pragma once

#include "config/ConfigManager.h"

class ServerApplication final
{
public:
    int run(const ServerConfig& config);
};
