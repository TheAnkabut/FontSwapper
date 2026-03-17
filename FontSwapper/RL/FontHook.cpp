#include "pch.h"
#include "FontHook.h"
#include "../UI/Settings.h"
#include <vector>

#define MH_STATIC
#include "../MinHook/MinHook.h"

#pragma comment(lib, "gdi32.lib")

namespace FontHook
{
    // ==================== Function Finder ==================== 

    static uintptr_t FindFontLookupAddress(uintptr_t baseAddress, size_t imageSize) {
        const char* searchStr = "Searching for font: \"";
        size_t patternLen = strlen(searchStr);
        const uint8_t* data = reinterpret_cast<const uint8_t*>(baseAddress);

        for (size_t i = 0; i < imageSize - patternLen; ++i) {
            // 1. String
            if (memcmp(data + i, searchStr, patternLen) == 0) {
                uintptr_t strAddr = baseAddress + i;

                // 2. Xref
                for (size_t j = 0; j < imageSize - 7; ++j) {
                    uintptr_t xrefAddr = 0;
                    if (j + 8 <= imageSize && *reinterpret_cast<const uintptr_t*>(data + j) == strAddr) {
                        xrefAddr = baseAddress + j;
                    } else if (j + 7 <= imageSize) {
                        int32_t offset = *reinterpret_cast<const int32_t*>(data + j + 3);
                        if ((baseAddress + j) + 7 + offset == strAddr) {
                            xrefAddr = baseAddress + j;
                        }
                    }

                    // 3. Function start
                    if (xrefAddr != 0) {
                        size_t offset = xrefAddr - baseAddress;
                        for (size_t k = 0; k < 2000 && offset >= k; ++k) {
                            if (offset - k > 0 && data[offset - k - 1] == 0xCC) {
                                return baseAddress + offset - k;
                            }
                        }
                    }
                }
            }
        }
        return 0;
    }

    // ==================== State ====================

    static uintptr_t fontFunctionAddress = 0;

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
                if (redirectedFont == "None") {
                    fontToUse = "";
                } else {
                    fontToUse = redirectedFont.c_str();
                }
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

        uintptr_t functionAddress = FindFontLookupAddress(baseAddress, imageSize);

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
