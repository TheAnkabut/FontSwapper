#pragma once

#include <string>
#include <unordered_map>

// Localization - Spanish/English
// Use L("key") to get translated strings

namespace Localization
{
    inline bool isSpanish = false;
    
    inline void Initialize(std::shared_ptr<GameWrapper> wrapper)
    {
        isSpanish = (wrapper->GetUILanguage().ToString() == "ESN");
    }
    
    inline const char* Get(const char* key)
    {
        // Spanish
        static const std::unordered_map<std::string, const char*> ES = {
            // Header
            {"join_discord", "¡Haz clic para unirte al Discord!"},
            {"title", "Font Swapper"},
            {"status", "Estado"},

            {"hook_failed", "Error al hookear"},
            {"hook_failed_desc", "No se encontró la función de fuentes. El juego puede haberse actualizado."},
            
            // Font mapping table
            {"font_mappings", "Mapeo de Fuentes"},
            {"source_font", "Fuente Origen"},
            {"target_font", "Fuente Destino"},
            {"keep_original", "(mantener original)"},
            
            // Actions
            {"reset_all", "Resetear Todo"},
            {"set_all_to", "Poner Todas a..."},
            
            // Notes
            {"changes_note", "Los cambios pueden tardar. Navega o juega para aplicarlos."},
        };
        
        // English
        static const std::unordered_map<std::string, const char*> EN = {
            // Header
            {"join_discord", "Click to join the Discord!"},
            {"title", "Font Swapper"},
            {"status", "Status"},

            {"hook_failed", "Hook Failed"},
            {"hook_failed_desc", "Could not find font function. Game may have been updated."},
            
            // Font mapping table
            {"font_mappings", "Font Mappings"},
            {"source_font", "Source Font"},
            {"target_font", "Target Font"},
            {"keep_original", "(keep original)"},
            
            // Actions
            {"reset_all", "Reset All"},
            {"set_all_to", "Set All To..."},
            
            // Notes
            {"changes_note", "Changes may take a moment. Navigate or play to apply."},
        };
        
        const auto& dict = isSpanish ? ES : EN;
        auto it = dict.find(key);
        return (it != dict.end()) ? it->second : key;
    }
}

// Shortcut macro
#define L(key) Localization::Get(key)
