#include "pch.h"
#include "FontHook.h"
#include "../UI/Settings.h"
#include <vector>

#define MH_STATIC
#include "../MinHook/MinHook.h"

#pragma comment(lib, "gdi32.lib")

namespace FontHook
{
    // ==================== State ====================

    static uintptr_t fontFunctionAddress = 0;

    // Pattern of the font lookup 
    static const std::vector<uint8_t> FontFunctionSignature = {
        0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x4C,
        0x24, 0x08, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41,
        0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x81, 0xEC,
        0xA0, 0x00, 0x00, 0x00, 0x33, 0xDB
    };


    // Original function pointer (set by MinHook)
    typedef void* (__fastcall* FontLookupFunction)(
        void* context,
        const char* fontName,
        int flags,
        void* param4
    );
    static FontLookupFunction OriginalFontLookup = nullptr;

    // Pointer to the active font mappings (Settings)
    static const std::unordered_map<std::string, std::string>* activeMappings = nullptr;

    // ==================== Pattern Scanner ====================

    static uintptr_t FindPatternInMemory(
        uintptr_t searchStart,
        size_t searchSize,
        const std::vector<uint8_t>& pattern)
    {
        const uint8_t* memory = reinterpret_cast<const uint8_t*>(searchStart);
        size_t patternSize = pattern.size();

        for (size_t offset = 0; offset < searchSize - patternSize; ++offset)
        {
            bool matched = true;

            for (size_t i = 0; i < patternSize; ++i)
            {
                if (memory[offset + i] != pattern[i])
                {
                    matched = false;
                    break;
                }
            }

            if (matched)
            {
                return searchStart + offset;
            }
        }

        return 0;
    }

    // ==================== Hooked Font Lookup ====================

    static void* __fastcall HookedFontLookup(
        void* context,
        const char* requestedFontName,
        int flags,
        void* param4)
    {
        const char* fontToUse = requestedFontName;
        std::string redirectedFont;

        if (requestedFontName != nullptr && activeMappings != nullptr)
        {
            std::string requestedName(requestedFontName);

            auto it = activeMappings->find(requestedName);
            if (it != activeMappings->end() && !it->second.empty())
            {
                redirectedFont = it->second;
                fontToUse = redirectedFont.c_str();
            }
        }

        return OriginalFontLookup(context, fontToUse, flags, param4);
    }

    // ==================== Public API ====================

    bool Initialize(uintptr_t baseAddress, size_t imageSize,
                    const std::unordered_map<std::string, std::string>& fontMappings)
    {
        activeMappings = &fontMappings;

        if (OriginalFontLookup != nullptr)
        {
            return true;
        }

        uintptr_t functionAddress = FindPatternInMemory(
            baseAddress,
            imageSize,
            FontFunctionSignature
        );

        if (functionAddress == 0)
        {
            return false;
        }

        MH_STATUS createResult = MH_CreateHook(
            reinterpret_cast<LPVOID>(functionAddress),
            &HookedFontLookup,
            reinterpret_cast<LPVOID*>(&OriginalFontLookup)
        );

        if (createResult != MH_OK)
        {
            return false;
        }

        MH_STATUS enableResult = MH_EnableHook(
            reinterpret_cast<LPVOID>(functionAddress)
        );

        if (enableResult != MH_OK)
        {
            return false;
        }

        fontFunctionAddress = functionAddress;
        return true;
    }

    void Shutdown()
    {
        if (fontFunctionAddress != 0)
        {
            MH_DisableHook(reinterpret_cast<LPVOID>(fontFunctionAddress));
            fontFunctionAddress = 0;
        }

        MH_Uninitialize();
        OriginalFontLookup = nullptr;
        activeMappings = nullptr;
    }

    bool IsActive()
    {
        return OriginalFontLookup != nullptr;
    }
}
