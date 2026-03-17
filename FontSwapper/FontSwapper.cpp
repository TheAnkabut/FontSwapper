
#include "pch.h"
#include "FontSwapper.h"
#include "UI/Settings.h"
#include "RL/FontHook.h"
#include "Utils/Localization.h"
#include <Windows.h>
#include <Psapi.h>

#define MH_STATIC
#include "MinHook/MinHook.h"

BAKKESMOD_PLUGIN(FontSwapper, "Font Swapper", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

// ==================== Plugin Lifecycle ====================

void FontSwapper::onLoad()
{
    _globalCvarManager = cvarManager;

    // Count
    SendCount();

    // MinHook
    if (MH_Initialize() != MH_OK)
    {
        LOG("FontSwapper: Failed to initialize MinHook");
        return;
    }

    // Game module
    HMODULE gameModule = GetModuleHandleW(L"RocketLeague.exe");
    if (gameModule == nullptr)
    {
        LOG("FontSwapper: Could not find game module");
        return;
    }

    MODULEINFO moduleInfo;
    BOOL gotInfo = GetModuleInformation(
        GetCurrentProcess(),
        gameModule,
        &moduleInfo,
        sizeof(moduleInfo)
    );

    if (!gotInfo)
    {
        LOG("FontSwapper: Could not get module info");
        return;
    }

    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(gameModule);
    size_t imageSize = moduleInfo.SizeOfImage;

    Localization::Initialize(gameWrapper);

    // Font mapping CVars
    for (const auto& font : Settings::knownFonts)
    {
        std::string clean = font;
        if (!clean.empty() && clean[0] == '$') clean = clean.substr(1);
        std::string cvarName = "fs_map_" + clean;
        cvarManager->registerCvar(cvarName, "0", "Font mapping for " + font, true, true, 0, true, static_cast<float>(Settings::knownFonts.size() + 1), true);
    }

    Settings::Initialize();

    // Font hook
    Settings::hookActive = FontHook::Initialize(baseAddress, imageSize, Settings::fontMappings);

    if (!Settings::hookActive)
    {
        LOG("FontSwapper: Hook installation failed");
    }

    gameWrapper->SetTimeout([](GameWrapper*) {
        Settings::Initialize();
    }, 3.0f);
}

void FontSwapper::onUnload()
{
    FontHook::Shutdown();
}

void FontSwapper::RenderSettings()
{
    // Discord link
    ImVec4 discordColor = ImVec4(0.30f, 0.85f, 1.0f, 1.0f);
    ImGui::TextColored(discordColor, L("join_discord"));
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(min.x, max.y), ImVec2(max.x, max.y), ImGui::GetColorU32(discordColor), 1.0f);
    }
    if (ImGui::IsItemClicked())
    {
        ShellExecuteA(nullptr, "open", "https://discord.gg/FPvkjaBPEA", nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    ImGui::Text("·");
    ImGui::SameLine();
    ImGui::Text("Made by The Ankabut");

    Settings::Render();
}

// ==================== Count ====================

void FontSwapper::SendCount()
{
    cvarManager->registerCvar("fontswapper_counted", "0", "Count flag", true, true, 0, true, 1, true);

    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        auto sentCvar = cvarManager->getCvar("fontswapper_counted");
        if (!sentCvar || sentCvar.getIntValue() == 1)
        {
            LOG("[Count] Already sent, skipping.");
            return;
        }

        // Player ID
        std::string idRL;
        if (gameWrapper->IsUsingEpicVersion()) {
            idRL = gameWrapper->GetEpicID();
        } else {
            idRL = std::to_string(gameWrapper->GetSteamID());
        }

        // Player name
        std::string playerName = gameWrapper->GetPlayerName().ToString();

        // Request body
        std::string body = "{\"pluginName\":\"Font Swapper\",\"idRL\":\"" + idRL + "\",\"playerName\":\"" + playerName + "\"}";

        CurlRequest req;
        req.url = "https://script.google.com/macros/s/AKfycbyudCL3iGwkhkoCGVDpIL7FOOkXrgTkDjd8g6yJtIz7qwiQ11IOoebbwD5YFL4ctayU/exec";
        req.body = body;

        HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {
            if (code == 0 || result == "error") {
                LOG("[Count] Failed, will retry next load.");
                return;
            }
            auto sentCvar = cvarManager->getCvar("fontswapper_counted");
            if (sentCvar) {
                sentCvar.setValue(1);
                cvarManager->executeCommand("writeconfig", false);
                LOG("[Count] Sent successfully. Total: {}", result);
            }
        });
    }, 7.0f);
}
