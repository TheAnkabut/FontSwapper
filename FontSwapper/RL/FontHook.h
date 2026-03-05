#pragma once

#include <string>
#include <unordered_map>

namespace FontHook
{
    bool Initialize(uintptr_t baseAddress, size_t imageSize,
                    const std::unordered_map<std::string, std::string>& fontMappings);
    void Shutdown();
    bool IsActive();
}
