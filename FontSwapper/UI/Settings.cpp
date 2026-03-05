#include "pch.h"
#include "Settings.h"
#include "IMGUI/imgui.h"
#include "../Utils/Localization.h"

namespace Settings
{
    // ==================== State ====================
    
    bool hookActive = false;
    std::unordered_map<std::string, std::string> fontMappings;
    
    const std::vector<std::string> knownFonts = {
        "$NormalFont",
        "$HeaderFont", 
        "$SmallBoldFont",
        "$HeaderThinFont",
        "$SmallFont",
        "$NumbersWideFont"
    };

    // ==================== CVar Helpers ====================

    // CVar name for a given font alias ("$NormalFont" -> "fs_map_NormalFont")
    static std::string CvarNameFor(const std::string& fontAlias)
    {
        std::string clean = fontAlias;
        if (!clean.empty() && clean[0] == '$')
        {
            clean = clean.substr(1);
        }
        return "fs_map_" + clean;
    }

    // ==================== Initialization ====================
    
    void Initialize()
    {
        fontMappings.clear();
        
        for (const auto& font : knownFonts)
        {
            auto cvar = _globalCvarManager->getCvar(CvarNameFor(font));
            if (!cvar) continue;
            int idx = cvar.getIntValue();
            if (idx >= 1 && idx <= static_cast<int>(knownFonts.size()))
            {
                fontMappings[font] = knownFonts[idx - 1];
            }
        }
    }

    // ==================== ImGui Rendering ====================
    
    void Render()
    {
        Initialize();
        
        if (!hookActive)
        {
            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s: %s", L("status"), L("hook_failed"));
            ImGui::Spacing();
            ImGui::TextWrapped("%s", L("hook_failed_desc"));
            return;
        }
        
        ImGui::Spacing();
        
        // ==================== Per-font Mapping Table ====================
        
        ImGui::TextUnformatted(L("font_mappings"));
        ImGui::Spacing();
        
        int fontCount = static_cast<int>(knownFonts.size());

        ImGui::Columns(2, "FontMappingColumns", true);
        ImGui::SetColumnWidth(0, 150.0f);
        ImGui::TextUnformatted(L("source_font"));
        ImGui::NextColumn();
        ImGui::TextUnformatted(L("target_font"));
        ImGui::NextColumn();
        ImGui::Separator();

        for (int i = 0; i < fontCount; ++i)
        {
            const std::string& sourceFont = knownFonts[i];
            
            // Current mapping (empty = original)
            std::string currentTarget = "";
            auto it = fontMappings.find(sourceFont);
            if (it != fontMappings.end())
            {
                currentTarget = it->second;
            }
            
            ImGui::TextUnformatted(sourceFont.c_str());
            ImGui::NextColumn();
            
            const char* previewText = currentTarget.empty() 
                ? L("keep_original") 
                : currentTarget.c_str();
            
            std::string comboId = "##target_" + std::to_string(i);
            ImGui::SetNextItemWidth(-1);
            
            if (ImGui::BeginCombo(comboId.c_str(), previewText))
            {
                bool isOriginal = currentTarget.empty();
                if (ImGui::Selectable(L("keep_original"), isOriginal))
                {
                    fontMappings.erase(sourceFont);
                    auto cvar = _globalCvarManager->getCvar(CvarNameFor(sourceFont));
                    if (cvar) cvar.setValue(0);
                    _globalCvarManager->executeCommand("writeconfig", false);
                }
                if (isOriginal)
                {
                    ImGui::SetItemDefaultFocus();
                }
                
                ImGui::Separator();
                
                // All known fonts
                for (int j = 0; j < fontCount; ++j)
                {
                    const std::string& targetFont = knownFonts[j];
                    bool isSelected = (currentTarget == targetFont);
                    
                    if (ImGui::Selectable(targetFont.c_str(), isSelected))
                    {
                        fontMappings[sourceFont] = targetFont;
                        auto cvar = _globalCvarManager->getCvar(CvarNameFor(sourceFont));
                        if (cvar) cvar.setValue(j + 1);
                        _globalCvarManager->executeCommand("writeconfig", false);
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                
                ImGui::EndCombo();
            }
            
            ImGui::NextColumn();
        }
        
        ImGui::Columns(1);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // ==================== Quick Actions ====================
        
        if (ImGui::Button(L("reset_all")))
        {
            fontMappings.clear();
            for (const auto& font : knownFonts)
            {
                auto cvar = _globalCvarManager->getCvar(CvarNameFor(font));
                if (cvar) cvar.setValue(0);
            }
            _globalCvarManager->executeCommand("writeconfig", false);
        }
        
        ImGui::SameLine();
        
        if (ImGui::BeginCombo("##setall", L("set_all_to")))
        {
            for (int j = 0; j < fontCount; ++j)
            {
                const std::string& targetFont = knownFonts[j];
                if (ImGui::Selectable(targetFont.c_str(), false))
                {
                    for (const auto& srcFont : knownFonts)
                    {
                        fontMappings[srcFont] = targetFont;
                        auto cvar = _globalCvarManager->getCvar(CvarNameFor(srcFont));
                        if (cvar) cvar.setValue(j + 1);
                    }
                    _globalCvarManager->executeCommand("writeconfig", false);
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", L("changes_note"));
    }
}
