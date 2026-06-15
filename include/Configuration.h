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

    struct TierVisualSettings {
        std::string tierText;
        std::array<float, 4> letterColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        std::array<float, 4> tierEmptyColor{ 46.0f / 255.0f, 51.0f / 255.0f, 61.0f / 255.0f, 166.0f / 255.0f };
        std::array<float, 4> textColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        std::array<float, 4> numberColor{ 229.0f / 255.0f, 229.0f / 255.0f, 229.0f / 255.0f, 1.0f };
        std::array<float, 4> backgroundColor{ 5.0f / 255.0f, 5.0f / 255.0f, 5.0f / 255.0f, 140.0f / 255.0f };
        int textFontWeight = 2;
        int numberFontWeight = 2;
        bool textAllCaps = false;
        bool numberAllCaps = false;
    };

    struct PlayerUISettings {
        bool enabled = true;
        bool showFloatingMessages = true;
        bool showComboHits = true;
        bool showComboNumber = true;
        bool showTotalComboPoints = false;
        bool editMode = true;
        int progressDisplayMode = 3;
        bool showTierName = false;
        int positionXPercent = 100;
        int positionYPercent = 11;
        int scalePercent = 159;
        int progressBarWidth = 89;
        int progressBarHeight = 8;
        int comboLabelYOffset = -7;
        int comboNumberYOffset = -7;
        int tierYOffset = -10;
        int tierTextYOffset = 33;
        int progressBarYOffset = -42;
        int comboHitsYOffset = -23;
        int backgroundScalePercent = 106;
        int comboLabelScalePercent = 64;
        int comboNumberScalePercent = 74;
        int tierScalePercent = 41;
        int tierTextScalePercent = 71;
        int progressBarScalePercent = 134;
        int comboHitsScalePercent = 110;
        std::array<float, 4> notificationPositiveColor{ 0.97f, 0.93f, 0.54f, 1.0f };
        std::array<float, 4> notificationNegativeColor{ 1.0f, 0.62f, 0.62f, 1.0f };
        bool showNotificationValue = true;
        std::string notificationHitText = "Hit";
        std::string notificationHitTakenText = "Hit Taken";
        std::string notificationDodgeText = "Dodge";
        std::string notificationPerfectDodgeText = "Perfect Dodge";
        std::string notificationDodgedText = "Dodged";
        std::string notificationPerfectDodgedText = "Perfect Dodged";
        std::string notificationParryText = "Parry";
        std::string notificationPerfectParryText = "Perfect Parry";
        std::string notificationParriedText = "Parried";
        std::string notificationPerfectParriedText = "Perfect Parried";
        std::string notificationUndodgeableText = "Undodgeable";
        std::string notificationUndodgeableHitText = "Undodgeable Hit";
        std::string notificationUnblockableText = "Unblockable";
        std::string notificationUnblockableHitText = "Unblockable Hit";
        std::string notificationStaggerText = "Stagger";
        std::array<TierVisualSettings, kComboTierCount> tiers{};

        PlayerUISettings();
    };

    struct ComboRule {
        std::string ruleName = "New Rule";
        RE::FormID perkID = 0;
        ComboProfileSettings profile;
    };

    inline ComboProfileSettings PlayerCombo;
    inline ComboProfileSettings NPCCombo;
    inline PlayerUISettings PlayerUI;
    inline std::vector<ComboRule> ComboRules;
}

namespace ModMenu {
    void Register();
    void PlayerRender();
    void UIRender();
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
