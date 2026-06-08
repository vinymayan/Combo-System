#pragma once
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <array>

namespace Settings {
    inline constexpr int kComboTierCount = 10;

    inline const char* ComboTierNames[kComboTierCount] = {
        "F", "E", "D", "C", "B", "A", "S", "SS", "SSS", "Z"
    };

    struct TierSettings {
        int pointsPerTier = 100;
        int baseHitPoints = 10;
        int repeatHitPoints = 2;
        int variedHitPoints = 20;
        int getHitPenalty = 15;
        int sourceChangeBonus = 10;
        int sourceTypeChangeBonus = 15;
        int dodgePoints = 10;
        int perfectDodgePoints = 20;
        int gotDodgedPoints = -10;
        int gotPerfectDodgedPoints = -20;
        int parryPoints = 10;
        int perfectParryPoints = 20;
        int gotParriedPoints = -10;
        int gotPerfectParriedPoints = -20;
        int undodgeableHitPoints = 15;
        int hitByUndodgeablePoints = -15;
        int unblockableHitPoints = 15;
        int hitByUnblockablePoints = -15;
        int staggerStartPoints = -10;
        bool losePointsPerSecond = true;
        int pointsLostPerSecond = 5;
        bool requireMinHitsForTier = false;
        int minHitsForTier = 3;
    };

    struct ComboProfileSettings {
        bool enabled = true;
        int expireComboSeconds = 20;
        std::array<TierSettings, kComboTierCount> tiers{};
    };

    struct ComboRule {
        std::string ruleName = "New Rule";
        RE::FormID perkID = 0;
        ComboProfileSettings profile;
    };

    inline ComboProfileSettings PlayerCombo;
    inline ComboProfileSettings NPCCombo;
    inline std::vector<ComboRule> ComboRules;
}

namespace ModMenu {
    void Register();
    void PlayerRender();
    void NPCRender();
    void RulesRender();
    void LoadSettings();
    void SaveSettings();
    void LoadLanguage();
    const char* GetLoc(const std::string& key, const char* defaultVal);
    // Utilitários de JSON
    void ReadJSON(rapidjson::Document& doc, const char* nome, std::vector<int>& lista);
    void WriteJSON(rapidjson::Document& doc, rapidjson::Document::AllocatorType& alloc, const char* nome, const std::vector<int>& lista);
}
