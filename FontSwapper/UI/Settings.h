#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Settings
{
    void Initialize();
    void Render();
    
    // Hook status
    extern bool hookActive;
    
    // Per-font map: fontMappings[fontName] = targetFontName / Empty = keep original
    extern std::unordered_map<std::string, std::string> fontMappings;
    
    // List of known font aliases used by RL
    extern const std::vector<std::string> knownFonts;
}
