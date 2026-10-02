#include "Configuration.h"
#include "Manager.h"
#include "Prisma.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <cstring>
#include <filesystem>

namespace Settings {
    PlayerUISettings::PlayerUISettings() {
        const std::array<std::array<float, 4>, kComboTierCount> colors{ {
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f },
            { 1.00f, 1.00f, 1.00f, 1.0f }
        } };
        const std::array<const char*, kComboTierCount> defaultTierTexts{ {
            "Fierce", "Exalted", "Dauntless", "Champion", "Brilliant", "Ascendant", "Sovereign!", "Supreme Skill!", "Stellar S Style!", "Zusk!"
        } };

        for (int i = 0; i < kComboTierCount; i++) {
            tiers[i].tierText = defaultTierTexts[i];
            tiers[i].letterColor = colors[i];
            tiers[i].tierEmptyColor = { 46.0f / 255.0f, 51.0f / 255.0f, 61.0f / 255.0f, 166.0f / 255.0f };
            tiers[i].textColor = colors[i];
            tiers[i].numberColor = { 229.0f / 255.0f, 229.0f / 255.0f, 229.0f / 255.0f, 1.0f };
            tiers[i].backgroundColor = { 5.0f / 255.0f, 5.0f / 255.0f, 5.0f / 255.0f, 140.0f / 255.0f };
        }
    }
}

namespace ModMenu {
    constexpr const char* SETTINGS_PATH = "Data/Viny Mods/Combo System/Settings.json";
    constexpr const char* PLAYER_SETTINGS_PATH = "Data/Viny Mods/Combo System/PlayerSettings.json";
    constexpr const char* NPC_SETTINGS_PATH = "Data/Viny Mods/Combo System/NPCSettings.json";
    constexpr const char* UI_SETTINGS_PATH = "Data/Viny Mods/Combo System/UISettings.json";
    constexpr const char* LANG_PATH = "Data/Viny Mods/Combo System/Language.json";
    const std::string RULES_DIR = "Data/Viny Mods/Combo System/Rules/";
    static std::unordered_map<std::string, std::string> LangMap;

    void LoadLanguage() {
        LangMap.clear();

        std::ifstream file(LANG_PATH, std::ios::binary);
        if (!file.is_open()) {
            logger::warn("Combo System language file not found. Falling back to default text.");
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();

        if (jsonStr.size() >= 3 &&
            static_cast<unsigned char>(jsonStr[0]) == 0xEF &&
            static_cast<unsigned char>(jsonStr[1]) == 0xBB &&
            static_cast<unsigned char>(jsonStr[2]) == 0xBF) {
            jsonStr.erase(0, 3);
        }

        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            return;
        }

        for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
            if (itr->value.IsObject()) {
                const std::string category = itr->name.GetString();
                for (auto jtr = itr->value.MemberBegin(); jtr != itr->value.MemberEnd(); ++jtr) {
                    if (jtr->value.IsString()) {
                        LangMap[category + "." + jtr->name.GetString()] = jtr->value.GetString();
                    }
                }
            } else if (itr->value.IsString()) {
                LangMap[itr->name.GetString()] = itr->value.GetString();
            }
        }
    }

    const char* GetLoc(const std::string& key, const char* defaultVal) {
        auto it = LangMap.find(key);
        if (it != LangMap.end()) {
            return it->second.c_str();
        }
        return defaultVal;
    }

    static bool RenderIntSliderWithInput(const char* label, int* v, int vMin, int vMax) {
        bool changed = false;
        ImGui::PushID(label);

        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::SliderInt("##slider", v, vMin, vMax)) {
            changed = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputInt(label, v)) {
            changed = true;
        }

        if (*v < vMin) {
            *v = vMin;
            changed = true;
        } else if (*v > vMax) {
            *v = vMax;
            changed = true;
        }

        ImGui::PopID();
        return changed;
    }

    static bool RenderFontWeightCombo(const char* label, int& value) {
        bool changed = false;
        const char* items[] = {
            GetLoc("menu.font_weight_light", "Light"),
            GetLoc("menu.font_weight_medium", "Medium"),
            GetLoc("menu.font_weight_bold", "Bold")
        };
        value = std::clamp(value, 0, 2);
        constexpr int itemCount = static_cast<int>(std::size(items));
        if (ImGui::Combo(label, &value, items, itemCount)) {
            changed = true;
        }
        return changed;
    }

    static bool RenderStringInput(const char* label, std::string& value) {
        char buffer[128];
        strcpy_s(buffer, value.c_str());
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            value = buffer;
            return true;
        }
        return false;
    }

    static bool DrawDropdown(const char* label, const std::string& category, RE::FormID& currentFormID, float customWidth = -1.0f) {
        bool changed = false;
        const auto& fullList = Manager::GetSingleton()->GetList(category);
        if (fullList.empty()) {
            ImGui::Text("%s: %s", label, GetLoc("common.none", "None"));
            return false;
        }

        std::vector<const char*> comboItems;
        std::vector<int> mapToFull;

        comboItems.push_back(GetLoc("common.none", "None"));
        mapToFull.push_back(-1);

        int localSelection = 0;
        for (size_t i = 0; i < fullList.size(); ++i) {
            comboItems.push_back(fullList[i].cachedDisplayName.c_str());
            mapToFull.push_back(static_cast<int>(i));
            if (fullList[i].formID == currentFormID) {
                localSelection = static_cast<int>(i) + 1;
            }
        }

        ImGui::PushID(label);
        std::string displayLabel = label;
        if (auto hashPos = displayLabel.find("##"); hashPos != std::string::npos) {
            displayLabel = displayLabel.substr(0, hashPos);
        }

        ImGui::Text("%s:", displayLabel.c_str());
        ImGui::SameLine();
        if (customWidth > 0.0f) {
            ImGui::SetNextItemWidth(customWidth);
        }

        if (ImGui::BeginCombo("##drop", comboItems[localSelection])) {
            for (int i = 0; i < static_cast<int>(comboItems.size()); i++) {
                const bool isSelected = localSelection == i;
                if (ImGui::Selectable(comboItems[i], isSelected)) {
                    localSelection = i;
                    const int originalIndex = mapToFull[localSelection];
                    currentFormID = originalIndex == -1 ? 0 : fullList[originalIndex].formID;
                    changed = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PopID();
        return changed;
    }

    static void ClampTierSettings(Settings::TierSettings& settings) {
        settings.pointsPerTier = std::max(1, settings.pointsPerTier);
        settings.baseHitPoints = std::max(0, settings.baseHitPoints);
        settings.repeatHitPoints = std::max(0, settings.repeatHitPoints);
        settings.variedHitPoints = std::max(0, settings.variedHitPoints);
        settings.getHitPenalty = std::max(0, settings.getHitPenalty);
        settings.sourceChangeBonus = std::max(0, settings.sourceChangeBonus);
        settings.sourceTypeChangeBonus = std::max(0, settings.sourceTypeChangeBonus);
        settings.pointsLostPerSecond = std::max(0, settings.pointsLostPerSecond);
        settings.minHitsForTier = std::max(1, settings.minHitsForTier);
    }

    static void ReadTierSettings(const rapidjson::Value& parent, Settings::TierSettings& settings) {
        if (!parent.IsObject()) {
            return;
        }

        if (parent.HasMember("pointsPerTier") && parent["pointsPerTier"].IsInt()) settings.pointsPerTier = parent["pointsPerTier"].GetInt();
        if (parent.HasMember("baseHitPoints") && parent["baseHitPoints"].IsInt()) settings.baseHitPoints = parent["baseHitPoints"].GetInt();
        if (parent.HasMember("repeatHitPoints") && parent["repeatHitPoints"].IsInt()) settings.repeatHitPoints = parent["repeatHitPoints"].GetInt();
        if (parent.HasMember("variedHitPoints") && parent["variedHitPoints"].IsInt()) settings.variedHitPoints = parent["variedHitPoints"].GetInt();
        if (parent.HasMember("getHitPenalty") && parent["getHitPenalty"].IsInt()) settings.getHitPenalty = parent["getHitPenalty"].GetInt();
        if (parent.HasMember("sourceChangeBonus") && parent["sourceChangeBonus"].IsInt()) settings.sourceChangeBonus = parent["sourceChangeBonus"].GetInt();
        if (parent.HasMember("sourceTypeChangeBonus") && parent["sourceTypeChangeBonus"].IsInt()) settings.sourceTypeChangeBonus = parent["sourceTypeChangeBonus"].GetInt();
        if (parent.HasMember("dodgePoints") && parent["dodgePoints"].IsInt()) settings.dodgePoints = parent["dodgePoints"].GetInt();
        if (parent.HasMember("perfectDodgePoints") && parent["perfectDodgePoints"].IsInt()) settings.perfectDodgePoints = parent["perfectDodgePoints"].GetInt();
        if (parent.HasMember("gotDodgedPoints") && parent["gotDodgedPoints"].IsInt()) settings.gotDodgedPoints = parent["gotDodgedPoints"].GetInt();
        if (parent.HasMember("gotPerfectDodgedPoints") && parent["gotPerfectDodgedPoints"].IsInt()) settings.gotPerfectDodgedPoints = parent["gotPerfectDodgedPoints"].GetInt();
        if (parent.HasMember("parryPoints") && parent["parryPoints"].IsInt()) settings.parryPoints = parent["parryPoints"].GetInt();
        if (parent.HasMember("perfectParryPoints") && parent["perfectParryPoints"].IsInt()) settings.perfectParryPoints = parent["perfectParryPoints"].GetInt();
        if (parent.HasMember("gotParriedPoints") && parent["gotParriedPoints"].IsInt()) settings.gotParriedPoints = parent["gotParriedPoints"].GetInt();
        if (parent.HasMember("gotPerfectParriedPoints") && parent["gotPerfectParriedPoints"].IsInt()) settings.gotPerfectParriedPoints = parent["gotPerfectParriedPoints"].GetInt();
        if (parent.HasMember("undodgeableHitPoints") && parent["undodgeableHitPoints"].IsInt()) settings.undodgeableHitPoints = parent["undodgeableHitPoints"].GetInt();
        if (parent.HasMember("hitByUndodgeablePoints") && parent["hitByUndodgeablePoints"].IsInt()) settings.hitByUndodgeablePoints = parent["hitByUndodgeablePoints"].GetInt();
        if (parent.HasMember("unblockableHitPoints") && parent["unblockableHitPoints"].IsInt()) settings.unblockableHitPoints = parent["unblockableHitPoints"].GetInt();
        if (parent.HasMember("hitByUnblockablePoints") && parent["hitByUnblockablePoints"].IsInt()) settings.hitByUnblockablePoints = parent["hitByUnblockablePoints"].GetInt();
        if (parent.HasMember("staggerStartPoints") && parent["staggerStartPoints"].IsInt()) settings.staggerStartPoints = parent["staggerStartPoints"].GetInt();
        if (parent.HasMember("losePointsPerSecond") && parent["losePointsPerSecond"].IsBool()) settings.losePointsPerSecond = parent["losePointsPerSecond"].GetBool();
        if (parent.HasMember("pointsLostPerSecond") && parent["pointsLostPerSecond"].IsInt()) settings.pointsLostPerSecond = parent["pointsLostPerSecond"].GetInt();
        if (parent.HasMember("requireMinHitsForTier") && parent["requireMinHitsForTier"].IsBool()) settings.requireMinHitsForTier = parent["requireMinHitsForTier"].GetBool();
        if (parent.HasMember("minHitsForTier") && parent["minHitsForTier"].IsInt()) settings.minHitsForTier = parent["minHitsForTier"].GetInt();

        ClampTierSettings(settings);
    }

    static void WriteTierSettings(
        rapidjson::Value& parent,
        rapidjson::Document::AllocatorType& alloc,
        const Settings::TierSettings& settings) {
        parent.SetObject();
        parent.AddMember("pointsPerTier", settings.pointsPerTier, alloc);
        parent.AddMember("baseHitPoints", settings.baseHitPoints, alloc);
        parent.AddMember("repeatHitPoints", settings.repeatHitPoints, alloc);
        parent.AddMember("variedHitPoints", settings.variedHitPoints, alloc);
        parent.AddMember("getHitPenalty", settings.getHitPenalty, alloc);
        parent.AddMember("sourceChangeBonus", settings.sourceChangeBonus, alloc);
        parent.AddMember("sourceTypeChangeBonus", settings.sourceTypeChangeBonus, alloc);
        parent.AddMember("dodgePoints", settings.dodgePoints, alloc);
        parent.AddMember("perfectDodgePoints", settings.perfectDodgePoints, alloc);
        parent.AddMember("gotDodgedPoints", settings.gotDodgedPoints, alloc);
        parent.AddMember("gotPerfectDodgedPoints", settings.gotPerfectDodgedPoints, alloc);
        parent.AddMember("parryPoints", settings.parryPoints, alloc);
        parent.AddMember("perfectParryPoints", settings.perfectParryPoints, alloc);
        parent.AddMember("gotParriedPoints", settings.gotParriedPoints, alloc);
        parent.AddMember("gotPerfectParriedPoints", settings.gotPerfectParriedPoints, alloc);
        parent.AddMember("undodgeableHitPoints", settings.undodgeableHitPoints, alloc);
        parent.AddMember("hitByUndodgeablePoints", settings.hitByUndodgeablePoints, alloc);
        parent.AddMember("unblockableHitPoints", settings.unblockableHitPoints, alloc);
        parent.AddMember("hitByUnblockablePoints", settings.hitByUnblockablePoints, alloc);
        parent.AddMember("staggerStartPoints", settings.staggerStartPoints, alloc);
        parent.AddMember("losePointsPerSecond", settings.losePointsPerSecond, alloc);
        parent.AddMember("pointsLostPerSecond", settings.pointsLostPerSecond, alloc);
        parent.AddMember("requireMinHitsForTier", settings.requireMinHitsForTier, alloc);
        parent.AddMember("minHitsForTier", settings.minHitsForTier, alloc);
    }

    static void ClampPlayerUISettings(Settings::PlayerUISettings& settings) {
        settings.editPreviewTier = std::clamp(settings.editPreviewTier, 0, Settings::kComboTierCount - 1);
        settings.positionXPercent = std::clamp(settings.positionXPercent, 0, 100);
        settings.positionYPercent = std::clamp(settings.positionYPercent, 0, 100);
        settings.scalePercent = std::clamp(settings.scalePercent, 40, 300);
        settings.progressBarWidth = std::clamp(settings.progressBarWidth, 60, 600);
        settings.progressBarHeight = std::clamp(settings.progressBarHeight, 6, 60);
        settings.comboLabelYOffset = std::clamp(settings.comboLabelYOffset, -300, 300);
        settings.comboNumberYOffset = std::clamp(settings.comboNumberYOffset, -300, 300);
        settings.tierYOffset = std::clamp(settings.tierYOffset, -300, 300);
        settings.tierTextYOffset = std::clamp(settings.tierTextYOffset, -300, 300);
        settings.progressBarYOffset = std::clamp(settings.progressBarYOffset, -300, 300);
        settings.comboHitsYOffset = std::clamp(settings.comboHitsYOffset, -300, 300);
        settings.backgroundScalePercent = std::clamp(settings.backgroundScalePercent, 25, 300);
        settings.comboLabelScalePercent = std::clamp(settings.comboLabelScalePercent, 25, 300);
        settings.comboNumberScalePercent = std::clamp(settings.comboNumberScalePercent, 25, 300);
        settings.tierScalePercent = std::clamp(settings.tierScalePercent, 25, 300);
        settings.tierTextScalePercent = std::clamp(settings.tierTextScalePercent, 25, 300);
        settings.progressBarScalePercent = std::clamp(settings.progressBarScalePercent, 25, 300);
        settings.comboHitsScalePercent = std::clamp(settings.comboHitsScalePercent, 25, 300);
        settings.progressDisplayMode = std::clamp(settings.progressDisplayMode, 0, 3);
        for (auto& component : settings.notificationPositiveColor) {
            component = std::clamp(component, 0.0f, 1.0f);
        }
        for (auto& component : settings.notificationNegativeColor) {
            component = std::clamp(component, 0.0f, 1.0f);
        }
        for (auto& tier : settings.tiers) {
            for (auto& component : tier.letterColor) {
                component = std::clamp(component, 0.0f, 1.0f);
            }
            for (auto& component : tier.tierEmptyColor) {
                component = std::clamp(component, 0.0f, 1.0f);
            }
            for (auto& component : tier.textColor) {
                component = std::clamp(component, 0.0f, 1.0f);
            }
            for (auto& component : tier.numberColor) {
                component = std::clamp(component, 0.0f, 1.0f);
            }
            for (auto& component : tier.backgroundColor) {
                component = std::clamp(component, 0.0f, 1.0f);
            }
            tier.textFontWeight = std::clamp(tier.textFontWeight, 0, 2);
            tier.numberFontWeight = std::clamp(tier.numberFontWeight, 0, 2);
        }
    }

    static void ReadPlayerUISettings(const rapidjson::Value& parent, Settings::PlayerUISettings& settings) {
        if (!parent.IsObject()) {
            return;
        }

        if (parent.HasMember("showFloatingMessages") && parent["showFloatingMessages"].IsBool()) {
            settings.showFloatingMessages = parent["showFloatingMessages"].GetBool();
        }
        if (parent.HasMember("showComboHits") && parent["showComboHits"].IsBool()) {
            settings.showComboHits = parent["showComboHits"].GetBool();
        }
        if (parent.HasMember("showComboNumber") && parent["showComboNumber"].IsBool()) {
            settings.showComboNumber = parent["showComboNumber"].GetBool();
        }
        if (parent.HasMember("showTotalComboPoints") && parent["showTotalComboPoints"].IsBool()) {
            settings.showTotalComboPoints = parent["showTotalComboPoints"].GetBool();
        }
        if (parent.HasMember("enabled") && parent["enabled"].IsBool()) {
            settings.enabled = parent["enabled"].GetBool();
        }
        if (parent.HasMember("editMode") && parent["editMode"].IsBool()) {
            settings.editMode = parent["editMode"].GetBool();
        }
        if (parent.HasMember("editPreviewTier") && parent["editPreviewTier"].IsInt()) {
            settings.editPreviewTier = parent["editPreviewTier"].GetInt();
        }
        if (parent.HasMember("progressDisplayMode") && parent["progressDisplayMode"].IsInt()) {
            settings.progressDisplayMode = parent["progressDisplayMode"].GetInt();
        } else if (parent.HasMember("useTextProgressFill") && parent["useTextProgressFill"].IsBool()) {
            settings.progressDisplayMode = parent["useTextProgressFill"].GetBool() ? 2 : 1;
        }
        if (parent.HasMember("showTierName") && parent["showTierName"].IsBool()) {
            settings.showTierName = parent["showTierName"].GetBool();
        }
        if (parent.HasMember("positionXPercent") && parent["positionXPercent"].IsInt()) {
            settings.positionXPercent = parent["positionXPercent"].GetInt();
        }
        if (parent.HasMember("positionYPercent") && parent["positionYPercent"].IsInt()) {
            settings.positionYPercent = parent["positionYPercent"].GetInt();
        }
        if (parent.HasMember("scalePercent") && parent["scalePercent"].IsInt()) {
            settings.scalePercent = parent["scalePercent"].GetInt();
        }
        if (parent.HasMember("progressBarWidth") && parent["progressBarWidth"].IsInt()) {
            settings.progressBarWidth = parent["progressBarWidth"].GetInt();
        }
        if (parent.HasMember("progressBarHeight") && parent["progressBarHeight"].IsInt()) {
            settings.progressBarHeight = parent["progressBarHeight"].GetInt();
        }
        if (parent.HasMember("comboLabelYOffset") && parent["comboLabelYOffset"].IsInt()) {
            settings.comboLabelYOffset = parent["comboLabelYOffset"].GetInt();
        }
        if (parent.HasMember("comboNumberYOffset") && parent["comboNumberYOffset"].IsInt()) {
            settings.comboNumberYOffset = parent["comboNumberYOffset"].GetInt();
        }
        if (parent.HasMember("tierYOffset") && parent["tierYOffset"].IsInt()) {
            settings.tierYOffset = parent["tierYOffset"].GetInt();
        }
        if (parent.HasMember("tierTextYOffset") && parent["tierTextYOffset"].IsInt()) {
            settings.tierTextYOffset = parent["tierTextYOffset"].GetInt();
        }
        if (parent.HasMember("progressBarYOffset") && parent["progressBarYOffset"].IsInt()) {
            settings.progressBarYOffset = parent["progressBarYOffset"].GetInt();
        }
        if (parent.HasMember("comboHitsYOffset") && parent["comboHitsYOffset"].IsInt()) {
            settings.comboHitsYOffset = parent["comboHitsYOffset"].GetInt();
        }
        if (parent.HasMember("backgroundScalePercent") && parent["backgroundScalePercent"].IsInt()) {
            settings.backgroundScalePercent = parent["backgroundScalePercent"].GetInt();
        }
        if (parent.HasMember("comboLabelScalePercent") && parent["comboLabelScalePercent"].IsInt()) {
            settings.comboLabelScalePercent = parent["comboLabelScalePercent"].GetInt();
        }
        if (parent.HasMember("comboNumberScalePercent") && parent["comboNumberScalePercent"].IsInt()) {
            settings.comboNumberScalePercent = parent["comboNumberScalePercent"].GetInt();
        }
        if (parent.HasMember("tierScalePercent") && parent["tierScalePercent"].IsInt()) {
            settings.tierScalePercent = parent["tierScalePercent"].GetInt();
        }
        if (parent.HasMember("tierTextScalePercent") && parent["tierTextScalePercent"].IsInt()) {
            settings.tierTextScalePercent = parent["tierTextScalePercent"].GetInt();
        }
        if (parent.HasMember("progressBarScalePercent") && parent["progressBarScalePercent"].IsInt()) {
            settings.progressBarScalePercent = parent["progressBarScalePercent"].GetInt();
        }
        if (parent.HasMember("comboHitsScalePercent") && parent["comboHitsScalePercent"].IsInt()) {
            settings.comboHitsScalePercent = parent["comboHitsScalePercent"].GetInt();
        }
        if (parent.HasMember("notificationPositiveColor") && parent["notificationPositiveColor"].IsArray()) {
            int colorIndex = 0;
            for (const auto& component : parent["notificationPositiveColor"].GetArray()) {
                if (colorIndex >= 4) break;
                if (component.IsNumber()) settings.notificationPositiveColor[colorIndex] = component.GetFloat();
                colorIndex++;
            }
        }
        if (parent.HasMember("notificationNegativeColor") && parent["notificationNegativeColor"].IsArray()) {
            int colorIndex = 0;
            for (const auto& component : parent["notificationNegativeColor"].GetArray()) {
                if (colorIndex >= 4) break;
                if (component.IsNumber()) settings.notificationNegativeColor[colorIndex] = component.GetFloat();
                colorIndex++;
            }
        }
        if (parent.HasMember("showNotificationValue") && parent["showNotificationValue"].IsBool()) settings.showNotificationValue = parent["showNotificationValue"].GetBool();
        if (parent.HasMember("notificationHitText") && parent["notificationHitText"].IsString()) settings.notificationHitText = parent["notificationHitText"].GetString();
        if (parent.HasMember("notificationHitTakenText") && parent["notificationHitTakenText"].IsString()) settings.notificationHitTakenText = parent["notificationHitTakenText"].GetString();
        if (parent.HasMember("notificationDodgeText") && parent["notificationDodgeText"].IsString()) settings.notificationDodgeText = parent["notificationDodgeText"].GetString();
        if (parent.HasMember("notificationPerfectDodgeText") && parent["notificationPerfectDodgeText"].IsString()) settings.notificationPerfectDodgeText = parent["notificationPerfectDodgeText"].GetString();
        if (parent.HasMember("notificationDodgedText") && parent["notificationDodgedText"].IsString()) settings.notificationDodgedText = parent["notificationDodgedText"].GetString();
        if (parent.HasMember("notificationPerfectDodgedText") && parent["notificationPerfectDodgedText"].IsString()) settings.notificationPerfectDodgedText = parent["notificationPerfectDodgedText"].GetString();
        if (parent.HasMember("notificationParryText") && parent["notificationParryText"].IsString()) settings.notificationParryText = parent["notificationParryText"].GetString();
        if (parent.HasMember("notificationPerfectParryText") && parent["notificationPerfectParryText"].IsString()) settings.notificationPerfectParryText = parent["notificationPerfectParryText"].GetString();
        if (parent.HasMember("notificationParriedText") && parent["notificationParriedText"].IsString()) settings.notificationParriedText = parent["notificationParriedText"].GetString();
        if (parent.HasMember("notificationPerfectParriedText") && parent["notificationPerfectParriedText"].IsString()) settings.notificationPerfectParriedText = parent["notificationPerfectParriedText"].GetString();
        if (parent.HasMember("notificationUndodgeableText") && parent["notificationUndodgeableText"].IsString()) settings.notificationUndodgeableText = parent["notificationUndodgeableText"].GetString();
        if (parent.HasMember("notificationUndodgeableHitText") && parent["notificationUndodgeableHitText"].IsString()) settings.notificationUndodgeableHitText = parent["notificationUndodgeableHitText"].GetString();
        if (parent.HasMember("notificationUnblockableText") && parent["notificationUnblockableText"].IsString()) settings.notificationUnblockableText = parent["notificationUnblockableText"].GetString();
        if (parent.HasMember("notificationUnblockableHitText") && parent["notificationUnblockableHitText"].IsString()) settings.notificationUnblockableHitText = parent["notificationUnblockableHitText"].GetString();
        if (parent.HasMember("notificationStaggerText") && parent["notificationStaggerText"].IsString()) settings.notificationStaggerText = parent["notificationStaggerText"].GetString();

        if (parent.HasMember("tiers") && parent["tiers"].IsArray()) {
            int idx = 0;
            for (const auto& tierValue : parent["tiers"].GetArray()) {
                if (idx >= Settings::kComboTierCount) {
                    break;
                }
                if (tierValue.IsObject()) {
                    if (tierValue.HasMember("tierText") && tierValue["tierText"].IsString()) {
                        settings.tiers[idx].tierText = tierValue["tierText"].GetString();
                    }
                    if (tierValue.HasMember("letterColor") && tierValue["letterColor"].IsArray()) {
                        int colorIndex = 0;
                        for (const auto& component : tierValue["letterColor"].GetArray()) {
                            if (colorIndex >= 4) {
                                break;
                            }
                            if (component.IsNumber()) {
                                settings.tiers[idx].letterColor[colorIndex] = component.GetFloat();
                            }
                            colorIndex++;
                        }
                    }
                    if (tierValue.HasMember("backgroundColor") && tierValue["backgroundColor"].IsArray()) {
                        int colorIndex = 0;
                        for (const auto& component : tierValue["backgroundColor"].GetArray()) {
                            if (colorIndex >= 4) {
                                break;
                            }
                            if (component.IsNumber()) {
                                settings.tiers[idx].backgroundColor[colorIndex] = component.GetFloat();
                            }
                            colorIndex++;
                        }
                    }
                    if (tierValue.HasMember("tierEmptyColor") && tierValue["tierEmptyColor"].IsArray()) {
                        int colorIndex = 0;
                        for (const auto& component : tierValue["tierEmptyColor"].GetArray()) {
                            if (colorIndex >= 4) {
                                break;
                            }
                            if (component.IsNumber()) {
                                settings.tiers[idx].tierEmptyColor[colorIndex] = component.GetFloat();
                            }
                            colorIndex++;
                        }
                    }
                    if (tierValue.HasMember("textColor") && tierValue["textColor"].IsArray()) {
                        int colorIndex = 0;
                        for (const auto& component : tierValue["textColor"].GetArray()) {
                            if (colorIndex >= 4) {
                                break;
                            }
                            if (component.IsNumber()) {
                                settings.tiers[idx].textColor[colorIndex] = component.GetFloat();
                            }
                            colorIndex++;
                        }
                    }
                    if (tierValue.HasMember("numberColor") && tierValue["numberColor"].IsArray()) {
                        int colorIndex = 0;
                        for (const auto& component : tierValue["numberColor"].GetArray()) {
                            if (colorIndex >= 4) {
                                break;
                            }
                            if (component.IsNumber()) {
                                settings.tiers[idx].numberColor[colorIndex] = component.GetFloat();
                            }
                            colorIndex++;
                        }
                    }
                    if (tierValue.HasMember("textFontWeight") && tierValue["textFontWeight"].IsInt()) {
                        settings.tiers[idx].textFontWeight = tierValue["textFontWeight"].GetInt();
                    }
                    if (tierValue.HasMember("numberFontWeight") && tierValue["numberFontWeight"].IsInt()) {
                        settings.tiers[idx].numberFontWeight = tierValue["numberFontWeight"].GetInt();
                    }
                    if (tierValue.HasMember("textAllCaps") && tierValue["textAllCaps"].IsBool()) {
                        settings.tiers[idx].textAllCaps = tierValue["textAllCaps"].GetBool();
                    }
                    if (tierValue.HasMember("numberAllCaps") && tierValue["numberAllCaps"].IsBool()) {
                        settings.tiers[idx].numberAllCaps = tierValue["numberAllCaps"].GetBool();
                    }
                }
                idx++;
            }
        }

        ClampPlayerUISettings(settings);
    }

    static void WritePlayerUISettings(
        rapidjson::Value& parent,
        rapidjson::Document::AllocatorType& alloc,
        const Settings::PlayerUISettings& settings) {
        parent.SetObject();
        parent.AddMember("enabled", settings.enabled, alloc);
        parent.AddMember("showFloatingMessages", settings.showFloatingMessages, alloc);
        parent.AddMember("showComboHits", settings.showComboHits, alloc);
        parent.AddMember("showComboNumber", settings.showComboNumber, alloc);
        parent.AddMember("showTotalComboPoints", settings.showTotalComboPoints, alloc);
        parent.AddMember("editMode", settings.editMode, alloc);
        parent.AddMember("editPreviewTier", settings.editPreviewTier, alloc);
        parent.AddMember("progressDisplayMode", settings.progressDisplayMode, alloc);
        parent.AddMember("showTierName", settings.showTierName, alloc);
        parent.AddMember("positionXPercent", settings.positionXPercent, alloc);
        parent.AddMember("positionYPercent", settings.positionYPercent, alloc);
        parent.AddMember("scalePercent", settings.scalePercent, alloc);
        parent.AddMember("progressBarWidth", settings.progressBarWidth, alloc);
        parent.AddMember("progressBarHeight", settings.progressBarHeight, alloc);
        parent.AddMember("comboLabelYOffset", settings.comboLabelYOffset, alloc);
        parent.AddMember("comboNumberYOffset", settings.comboNumberYOffset, alloc);
        parent.AddMember("tierYOffset", settings.tierYOffset, alloc);
        parent.AddMember("tierTextYOffset", settings.tierTextYOffset, alloc);
        parent.AddMember("progressBarYOffset", settings.progressBarYOffset, alloc);
        parent.AddMember("comboHitsYOffset", settings.comboHitsYOffset, alloc);
        parent.AddMember("backgroundScalePercent", settings.backgroundScalePercent, alloc);
        parent.AddMember("comboLabelScalePercent", settings.comboLabelScalePercent, alloc);
        parent.AddMember("comboNumberScalePercent", settings.comboNumberScalePercent, alloc);
        parent.AddMember("tierScalePercent", settings.tierScalePercent, alloc);
        parent.AddMember("tierTextScalePercent", settings.tierTextScalePercent, alloc);
        parent.AddMember("progressBarScalePercent", settings.progressBarScalePercent, alloc);
        parent.AddMember("comboHitsScalePercent", settings.comboHitsScalePercent, alloc);
        rapidjson::Value notificationPositiveColor(rapidjson::kArrayType);
        for (float component : settings.notificationPositiveColor) {
            notificationPositiveColor.PushBack(component, alloc);
        }
        parent.AddMember("notificationPositiveColor", notificationPositiveColor, alloc);
        rapidjson::Value notificationNegativeColor(rapidjson::kArrayType);
        for (float component : settings.notificationNegativeColor) {
            notificationNegativeColor.PushBack(component, alloc);
        }
        parent.AddMember("notificationNegativeColor", notificationNegativeColor, alloc);
        parent.AddMember("showNotificationValue", settings.showNotificationValue, alloc);
        parent.AddMember("notificationHitText", rapidjson::Value(settings.notificationHitText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationHitTakenText", rapidjson::Value(settings.notificationHitTakenText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationDodgeText", rapidjson::Value(settings.notificationDodgeText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationPerfectDodgeText", rapidjson::Value(settings.notificationPerfectDodgeText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationDodgedText", rapidjson::Value(settings.notificationDodgedText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationPerfectDodgedText", rapidjson::Value(settings.notificationPerfectDodgedText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationParryText", rapidjson::Value(settings.notificationParryText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationPerfectParryText", rapidjson::Value(settings.notificationPerfectParryText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationParriedText", rapidjson::Value(settings.notificationParriedText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationPerfectParriedText", rapidjson::Value(settings.notificationPerfectParriedText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationUndodgeableText", rapidjson::Value(settings.notificationUndodgeableText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationUndodgeableHitText", rapidjson::Value(settings.notificationUndodgeableHitText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationUnblockableText", rapidjson::Value(settings.notificationUnblockableText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationUnblockableHitText", rapidjson::Value(settings.notificationUnblockableHitText.c_str(), alloc).Move(), alloc);
        parent.AddMember("notificationStaggerText", rapidjson::Value(settings.notificationStaggerText.c_str(), alloc).Move(), alloc);

        rapidjson::Value tiers(rapidjson::kArrayType);
        for (int i = 0; i < Settings::kComboTierCount; i++) {
            rapidjson::Value tierObj(rapidjson::kObjectType);
            tierObj.AddMember("name", rapidjson::Value(Settings::ComboTierNames[i], alloc).Move(), alloc);
            tierObj.AddMember("tierText", rapidjson::Value(settings.tiers[i].tierText.c_str(), alloc).Move(), alloc);

            rapidjson::Value color(rapidjson::kArrayType);
            for (float component : settings.tiers[i].letterColor) {
                color.PushBack(component, alloc);
            }
            tierObj.AddMember("letterColor", color, alloc);

            rapidjson::Value tierEmptyColor(rapidjson::kArrayType);
            for (float component : settings.tiers[i].tierEmptyColor) {
                tierEmptyColor.PushBack(component, alloc);
            }
            tierObj.AddMember("tierEmptyColor", tierEmptyColor, alloc);

            rapidjson::Value textColor(rapidjson::kArrayType);
            for (float component : settings.tiers[i].textColor) {
                textColor.PushBack(component, alloc);
            }
            tierObj.AddMember("textColor", textColor, alloc);

            rapidjson::Value numberColor(rapidjson::kArrayType);
            for (float component : settings.tiers[i].numberColor) {
                numberColor.PushBack(component, alloc);
            }
            tierObj.AddMember("numberColor", numberColor, alloc);

            rapidjson::Value backgroundColor(rapidjson::kArrayType);
            for (float component : settings.tiers[i].backgroundColor) {
                backgroundColor.PushBack(component, alloc);
            }
            tierObj.AddMember("backgroundColor", backgroundColor, alloc);
            tierObj.AddMember("textFontWeight", settings.tiers[i].textFontWeight, alloc);
            tierObj.AddMember("numberFontWeight", settings.tiers[i].numberFontWeight, alloc);
            tierObj.AddMember("textAllCaps", settings.tiers[i].textAllCaps, alloc);
            tierObj.AddMember("numberAllCaps", settings.tiers[i].numberAllCaps, alloc);

            tiers.PushBack(tierObj, alloc);
        }
        parent.AddMember("tiers", tiers, alloc);
    }

    static void ReadProfileSettings(const rapidjson::Value& parent, Settings::ComboProfileSettings& profile) {
        if (!parent.IsObject()) {
            return;
        }

        if (parent.HasMember("enabled") && parent["enabled"].IsBool()) {
            profile.enabled = parent["enabled"].GetBool();
        }
        if (parent.HasMember("expireComboSeconds") && parent["expireComboSeconds"].IsInt()) {
            profile.expireComboSeconds = std::max(0, parent["expireComboSeconds"].GetInt());
        }

        if (parent.HasMember("tiers") && parent["tiers"].IsArray()) {
            int idx = 0;
            for (const auto& tierValue : parent["tiers"].GetArray()) {
                if (idx >= Settings::kComboTierCount) {
                    break;
                }
                ReadTierSettings(tierValue, profile.tiers[idx]);
                idx++;
            }
            return;
        }

        Settings::TierSettings baseSettings;
        ReadTierSettings(parent, baseSettings);
        for (auto& tier : profile.tiers) {
            tier = baseSettings;
        }
    }

    static void WriteProfileSettings(
        rapidjson::Value& parent,
        rapidjson::Document::AllocatorType& alloc,
        const Settings::ComboProfileSettings& profile) {
        parent.SetObject();
        parent.AddMember("enabled", profile.enabled, alloc);
        parent.AddMember("expireComboSeconds", profile.expireComboSeconds, alloc);
        rapidjson::Value tiers(rapidjson::kArrayType);
        for (int i = 0; i < Settings::kComboTierCount; i++) {
            rapidjson::Value tierObj(rapidjson::kObjectType);
            tierObj.AddMember("name", rapidjson::Value(Settings::ComboTierNames[i], alloc).Move(), alloc);

            rapidjson::Value settingsObj(rapidjson::kObjectType);
            WriteTierSettings(settingsObj, alloc, profile.tiers[i]);
            tierObj.AddMember("settings", settingsObj, alloc);

            tiers.PushBack(tierObj, alloc);
        }
        parent.AddMember("tiers", tiers, alloc);
    }

    static void ReadProfileSettingsFromSavedValue(const rapidjson::Value& parent, Settings::ComboProfileSettings& profile) {
        if (!parent.IsObject()) {
            return;
        }

        if (parent.HasMember("tiers") && parent["tiers"].IsArray()) {
            if (parent.HasMember("enabled") && parent["enabled"].IsBool()) {
                profile.enabled = parent["enabled"].GetBool();
            }
            if (parent.HasMember("expireComboSeconds") && parent["expireComboSeconds"].IsInt()) {
                profile.expireComboSeconds = std::max(0, parent["expireComboSeconds"].GetInt());
            }
            int idx = 0;
            for (const auto& tierValue : parent["tiers"].GetArray()) {
                if (idx >= Settings::kComboTierCount) {
                    break;
                }
                if (tierValue.IsObject() && tierValue.HasMember("settings")) {
                    ReadTierSettings(tierValue["settings"], profile.tiers[idx]);
                } else {
                    ReadTierSettings(tierValue, profile.tiers[idx]);
                }
                idx++;
            }
            return;
        }

        ReadProfileSettings(parent, profile);
    }

    static bool RenderTierSettings(Settings::TierSettings& settings) {
        bool changed = false;

        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_progress_header", "Tier Progress"));
        if (RenderIntSliderWithInput(GetLoc("menu.points_per_tier", "Combo points required to advance tier"), &settings.pointsPerTier, 1, 2000)) changed = true;
        if (ImGui::Checkbox(GetLoc("menu.require_min_hits_for_tier", "Require a minimum hit count to advance tier"), &settings.requireMinHitsForTier)) changed = true;
        if (settings.requireMinHitsForTier) {
            if (RenderIntSliderWithInput(GetLoc("menu.min_hits_for_tier", "Minimum hits required to advance tier"), &settings.minHitsForTier, 1, 500)) changed = true;
        }

        ImGui::Separator();
        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_hit_points_header", "Hit Points"));
        if (RenderIntSliderWithInput(GetLoc("menu.base_hit_points", "Base hit points"), &settings.baseHitPoints, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.repeat_hit_points", "Repeated hit points"), &settings.repeatHitPoints, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.varied_hit_points", "Varied hit points"), &settings.variedHitPoints, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.get_hit_penalty", "Penalty when actor is hit"), &settings.getHitPenalty, 0, 500)) changed = true;

        ImGui::Separator();
        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_source_bonus_header", "Weapon/Magic Change Bonuses"));
        if (RenderIntSliderWithInput(GetLoc("menu.source_change_bonus", "Bonus when weapon/magic changes"), &settings.sourceChangeBonus, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.source_type_change_bonus", "Bonus when weapon/magic type changes"), &settings.sourceTypeChangeBonus, 0, 500)) changed = true;

        ImGui::Separator();
        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_dodge_events_header", "Dodge Events"));
        if (RenderIntSliderWithInput(GetLoc("menu.dodge_points", "Dodge event points"), &settings.dodgePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.perfect_dodge_points", "Perfect dodge event points"), &settings.perfectDodgePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_dodged_points", "Got dodged event points"), &settings.gotDodgedPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_perfect_dodged_points", "Got perfect dodged event points"), &settings.gotPerfectDodgedPoints, -500, 500)) changed = true;

        ImGui::Separator();
        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_parry_events_header", "Parry Events"));
        if (RenderIntSliderWithInput(GetLoc("menu.parry_points", "Parry event points"), &settings.parryPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.perfect_parry_points", "Perfect parry event points"), &settings.perfectParryPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_parried_points", "Got parried event points"), &settings.gotParriedPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_perfect_parried_points", "Got perfect parried event points"), &settings.gotPerfectParriedPoints, -500, 500)) changed = true;

        ImGui::Separator();
        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_special_events_header", "Special Events"));
        if (RenderIntSliderWithInput(GetLoc("menu.undodgeable_hit_points", "Undodgeable hit event points"), &settings.undodgeableHitPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.hit_by_undodgeable_points", "Hit by undodgeable event points"), &settings.hitByUndodgeablePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.unblockable_hit_points", "Unblockable hit event points"), &settings.unblockableHitPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.hit_by_unblockable_points", "Hit by unblockable event points"), &settings.hitByUnblockablePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.stagger_start_points", "Stagger start event points"), &settings.staggerStartPoints, -500, 500)) changed = true;

        ImGui::Separator();
        ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_decay_header", "Decay"));
        if (ImGui::Checkbox(GetLoc("menu.lose_points_per_second", "Lose combo points per second"), &settings.losePointsPerSecond)) changed = true;
        if (settings.losePointsPerSecond) {
            if (RenderIntSliderWithInput(GetLoc("menu.points_lost_per_second", "Combo points lost per second"), &settings.pointsLostPerSecond, 0, 500)) changed = true;
        }

        ClampTierSettings(settings);
        return changed;
    }

    static bool RenderProfileSettings(const char* sectionId, Settings::ComboProfileSettings& profile) {
        bool changed = false;
        ImGui::PushID(sectionId);

        if (ImGui::Checkbox(GetLoc("menu.profile_enabled", "Enable combo tracking"), &profile.enabled)) {
            changed = true;
        }
        if (RenderIntSliderWithInput(GetLoc("menu.expire_combo_seconds", "Combo expiration time (seconds)"), &profile.expireComboSeconds, 0, 300)) {
            changed = true;
        }
        ImGui::Separator();

        if (ImGui::Button(GetLoc("menu.copy_first_tier_to_all", "Copy F tier settings to all tiers"))) {
            for (int i = 1; i < Settings::kComboTierCount; i++) {
                profile.tiers[i] = profile.tiers[0];
            }
            changed = true;
        }

        for (int i = 0; i < Settings::kComboTierCount; i++) {
            std::string label = std::string(GetLoc("menu.tier", "Tier")) + " " + Settings::ComboTierNames[i];
            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::Indent();
                ImGui::PushID(i);
                if (RenderTierSettings(profile.tiers[i])) {
                    changed = true;
                }
                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        ImGui::PopID();
        return changed;
    }

    static bool RenderPlayerUISettings() {
        bool changed = false;
        auto& ui = Settings::PlayerUI;

        if (ImGui::CollapsingHeader(GetLoc("menu.ui_general_header", "General"))) {
            ImGui::Indent();
            if (ImGui::Checkbox(GetLoc("menu.ui_enabled", "Enable combo UI"), &ui.enabled)) changed = true;
            if (!ui.enabled) {
                ImGui::Text("%s", GetLoc("menu.ui_disabled_hint", "Combo UI is disabled."));
                ImGui::Unindent();
                ClampPlayerUISettings(ui);
                return changed;
            }
            if (ImGui::Checkbox(GetLoc("menu.ui_edit_mode", "Combo UI edit mode"), &ui.editMode)) changed = true;
            if (ui.editMode) {
                ui.editPreviewTier = std::clamp(ui.editPreviewTier, 0, Settings::kComboTierCount - 1);
                if (ImGui::Combo(GetLoc("menu.ui_edit_preview_tier", "Editor preview tier"), &ui.editPreviewTier,
                    Settings::ComboTierNames, Settings::kComboTierCount)) changed = true;
            }
            if (ImGui::Checkbox(GetLoc("menu.show_combo_number", "Show combo number"), &ui.showComboNumber)) changed = true;
            if (ImGui::Checkbox(GetLoc("menu.show_total_combo_points", "Show total combo points"), &ui.showTotalComboPoints)) changed = true;
            if (ImGui::Checkbox(GetLoc("menu.show_tier_name", "Show tier name text"), &ui.showTierName)) changed = true;
            if (ImGui::Checkbox(GetLoc("menu.show_combo_hits", "Show combo hits"), &ui.showComboHits)) changed = true;
            if (ImGui::Checkbox(GetLoc("menu.show_floating_messages", "Show floating combo messages"), &ui.showFloatingMessages)) changed = true;
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.ui_progress_header", "Progress"))) {
            ImGui::Indent();
            const char* progressModes[] = {
                GetLoc("menu.progress_display_none", "None"),
                GetLoc("menu.progress_display_bar", "Bar"),
                GetLoc("menu.progress_display_tier", "Tier"),
                GetLoc("menu.progress_display_both", "Both")
            };
            ui.progressDisplayMode = std::clamp(ui.progressDisplayMode, 0, 3);
            if (ImGui::Combo(GetLoc("menu.progress_display_mode", "Progress display mode"), &ui.progressDisplayMode, progressModes, 4)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.progress_bar_width", "Progress bar width"), &ui.progressBarWidth, 60, 600)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.progress_bar_height", "Progress bar height"), &ui.progressBarHeight, 6, 60)) changed = true;
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.ui_position_header", "Position and Base Size"))) {
            ImGui::Indent();
            if (RenderIntSliderWithInput(GetLoc("menu.ui_position_x", "Combo UI horizontal position (%)"), &ui.positionXPercent, 0, 100)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.ui_position_y", "Combo UI vertical position (%)"), &ui.positionYPercent, 0, 100)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.ui_scale", "Combo UI scale (%)"), &ui.scalePercent, 40, 300)) changed = true;
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.ui_offsets_header", "Vertical Offsets"))) {
            ImGui::Indent();
            if (RenderIntSliderWithInput(GetLoc("menu.combo_label_y_offset", "Combo label vertical offset"), &ui.comboLabelYOffset, -300, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.combo_number_y_offset", "Combo number vertical offset"), &ui.comboNumberYOffset, -300, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.tier_y_offset", "Tier vertical offset"), &ui.tierYOffset, -300, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.tier_text_y_offset", "Tier text vertical offset"), &ui.tierTextYOffset, -300, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.progress_bar_y_offset", "Progress bar vertical offset"), &ui.progressBarYOffset, -300, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.combo_hits_y_offset", "Combo hits vertical offset"), &ui.comboHitsYOffset, -300, 300)) changed = true;
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader(GetLoc("menu.ui_scales_header", "Element Scales"))) {
            ImGui::Indent();
            if (RenderIntSliderWithInput(GetLoc("menu.background_scale", "Background scale (%)"), &ui.backgroundScalePercent, 25, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.combo_label_scale", "Combo label scale (%)"), &ui.comboLabelScalePercent, 25, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.combo_number_scale", "Combo number scale (%)"), &ui.comboNumberScalePercent, 25, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.tier_scale", "Tier scale (%)"), &ui.tierScalePercent, 25, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.tier_text_scale", "Tier text scale (%)"), &ui.tierTextScalePercent, 25, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.progress_bar_scale", "Progress bar scale (%)"), &ui.progressBarScalePercent, 25, 300)) changed = true;
            if (RenderIntSliderWithInput(GetLoc("menu.combo_hits_scale", "Combo hits scale (%)"), &ui.comboHitsScalePercent, 25, 300)) changed = true;
            ImGui::Unindent();
        }

        if (ImGui::Button(GetLoc("menu.reset_ui_layout", "Reset UI layout"))) {
            ui.positionXPercent = 100;
            ui.positionYPercent = 11;
            ui.scalePercent = 159;
            ui.progressBarWidth = 89;
            ui.progressBarHeight = 8;
            ui.comboLabelYOffset = -7;
            ui.comboNumberYOffset = -7;
            ui.tierYOffset = -10;
            ui.tierTextYOffset = 33;
            ui.progressBarYOffset = -42;
            ui.comboHitsYOffset = -23;
            ui.backgroundScalePercent = 106;
            ui.comboLabelScalePercent = 64;
            ui.comboNumberScalePercent = 74;
            ui.tierScalePercent = 41;
            ui.tierTextScalePercent = 71;
            ui.progressBarScalePercent = 134;
            ui.comboHitsScalePercent = 110;
            changed = true;
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader(GetLoc("menu.notification_settings", "Notification Settings"))) {
            ImGui::Indent();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.notification_display_header", "Display"));
            if (ImGui::ColorEdit4(GetLoc("menu.notification_positive_color", "Positive notification color"), ui.notificationPositiveColor.data())) changed = true;
            if (ImGui::ColorEdit4(GetLoc("menu.notification_negative_color", "Negative notification color"), ui.notificationNegativeColor.data())) changed = true;
            if (ImGui::Checkbox(GetLoc("menu.show_notification_value", "Show notification value"), &ui.showNotificationValue)) changed = true;

            ImGui::Separator();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.notification_combat_header", "Combat Texts"));
            if (RenderStringInput(GetLoc("menu.notification_hit_text", "Hit notification text"), ui.notificationHitText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_hit_taken_text", "Hit taken notification text"), ui.notificationHitTakenText)) changed = true;

            ImGui::Separator();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.notification_dodge_header", "Dodge Texts"));
            if (RenderStringInput(GetLoc("menu.notification_dodge_text", "Dodge notification text"), ui.notificationDodgeText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_perfect_dodge_text", "Perfect dodge notification text"), ui.notificationPerfectDodgeText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_dodged_text", "Dodged notification text"), ui.notificationDodgedText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_perfect_dodged_text", "Perfect dodged notification text"), ui.notificationPerfectDodgedText)) changed = true;

            ImGui::Separator();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.notification_parry_header", "Parry Texts"));
            if (RenderStringInput(GetLoc("menu.notification_parry_text", "Parry notification text"), ui.notificationParryText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_perfect_parry_text", "Perfect parry notification text"), ui.notificationPerfectParryText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_parried_text", "Parried notification text"), ui.notificationParriedText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_perfect_parried_text", "Perfect parried notification text"), ui.notificationPerfectParriedText)) changed = true;

            ImGui::Separator();
            ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.notification_special_header", "Special Event Texts"));
            if (RenderStringInput(GetLoc("menu.notification_undodgeable_text", "Undodgeable notification text"), ui.notificationUndodgeableText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_undodgeable_hit_text", "Undodgeable hit notification text"), ui.notificationUndodgeableHitText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_unblockable_text", "Unblockable notification text"), ui.notificationUnblockableText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_unblockable_hit_text", "Unblockable hit notification text"), ui.notificationUnblockableHitText)) changed = true;
            if (RenderStringInput(GetLoc("menu.notification_stagger_text", "Stagger notification text"), ui.notificationStaggerText)) changed = true;
            ImGui::Unindent();
        }

        ImGui::Separator();
        for (int i = 0; i < Settings::kComboTierCount; i++) {
            std::string label = std::string(GetLoc("menu.tier_visual", "Tier Visual")) + " " + Settings::ComboTierNames[i];
            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::Indent();
                ImGui::PushID(i);

                ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_text_header", "Text"));
                if (RenderStringInput(GetLoc("menu.tier_custom_text", "Tier text"), ui.tiers[i].tierText)) {
                    changed = true;
                }
                if (RenderFontWeightCombo(GetLoc("menu.tier_text_font_weight", "Tier text font weight"), ui.tiers[i].textFontWeight)) {
                    changed = true;
                }
                if (RenderFontWeightCombo(GetLoc("menu.combo_number_font_weight", "Combo number font weight"), ui.tiers[i].numberFontWeight)) {
                    changed = true;
                }
                if (ImGui::Checkbox(GetLoc("menu.tier_text_all_caps", "Tier text all caps"), &ui.tiers[i].textAllCaps)) {
                    changed = true;
                }
                if (ImGui::Checkbox(GetLoc("menu.combo_number_all_caps", "Combo number all caps"), &ui.tiers[i].numberAllCaps)) {
                    changed = true;
                }

                ImGui::Separator();
                ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "%s", GetLoc("menu.tier_colors_header", "Colors"));
                if (ImGui::ColorEdit4(GetLoc("menu.tier_letter_color", "Tier letter color"), ui.tiers[i].letterColor.data())) {
                    changed = true;
                }
                if (ImGui::ColorEdit4(GetLoc("menu.tier_empty_color", "Tier empty color"), ui.tiers[i].tierEmptyColor.data())) {
                    changed = true;
                }
                if (ImGui::ColorEdit4(GetLoc("menu.tier_text_color", "Tier text color"), ui.tiers[i].textColor.data())) {
                    changed = true;
                }
                if (ImGui::ColorEdit4(GetLoc("menu.combo_number_color", "Combo number color"), ui.tiers[i].numberColor.data())) {
                    changed = true;
                }
                if (ImGui::ColorEdit4(GetLoc("menu.tier_background_color", "Tier background color"), ui.tiers[i].backgroundColor.data())) {
                    changed = true;
                }

                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        ClampPlayerUISettings(ui);
        return changed;
    }

    static void SaveRule(const Settings::ComboRule& rule) {
        std::filesystem::create_directories(RULES_DIR);

        const std::string filepath = RULES_DIR + rule.ruleName + ".json";
        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        doc.AddMember("ruleName", rapidjson::Value(rule.ruleName.c_str(), alloc).Move(), alloc);

        auto perkForm = RE::TESForm::LookupByID(rule.perkID);
        std::string perkStr = FormUtil::NormalizeFormID(perkForm);
        doc.AddMember("perk", rapidjson::Value(perkStr.c_str(), alloc).Move(), alloc);

        rapidjson::Value profileObj(rapidjson::kObjectType);
        WriteProfileSettings(profileObj, alloc, rule.profile);
        doc.AddMember("profile", profileObj, alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::ofstream file(filepath, std::ios::binary);
        if (file.is_open()) {
            file << buffer.GetString();
        }
    }

    static void LoadRules() {
        Settings::ComboRules.clear();
        if (!std::filesystem::exists(RULES_DIR)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(RULES_DIR)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") {
                continue;
            }

            std::ifstream file(entry.path(), std::ios::binary);
            if (!file.is_open()) {
                continue;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            rapidjson::Document doc;
            doc.Parse(buffer.str().c_str());
            if (doc.HasParseError() || !doc.IsObject()) {
                continue;
            }

            Settings::ComboRule rule;
            if (doc.HasMember("ruleName") && doc["ruleName"].IsString()) {
                rule.ruleName = doc["ruleName"].GetString();
            } else {
                rule.ruleName = entry.path().stem().string();
            }

            if (doc.HasMember("perk")) {
                if (doc["perk"].IsString()) {
                    rule.perkID = FormUtil::FormIDFromString(doc["perk"].GetString());
                } else if (doc["perk"].IsUint()) {
                    rule.perkID = doc["perk"].GetUint();
                }
            }

            if (doc.HasMember("profile") && doc["profile"].IsObject()) {
                ReadProfileSettingsFromSavedValue(doc["profile"], rule.profile);
            } else {
                ReadProfileSettingsFromSavedValue(doc, rule.profile);
            }

            Settings::ComboRules.push_back(rule);
        }
    }

    static bool LoadDocumentFromFile(const char* path, rapidjson::Document& doc) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();

        if (jsonStr.size() >= 3 &&
            static_cast<unsigned char>(jsonStr[0]) == 0xEF &&
            static_cast<unsigned char>(jsonStr[1]) == 0xBB &&
            static_cast<unsigned char>(jsonStr[2]) == 0xBF) {
            jsonStr.erase(0, 3);
        }

        doc.Parse(jsonStr.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            logger::warn("Combo System settings file could not be parsed: {}", path);
            return false;
        }

        return true;
    }

    static void WriteDocumentToFile(const char* path, const rapidjson::Document& doc) {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::ofstream file(path, std::ios::binary);
        if (file.is_open()) {
            file << buffer.GetString();
            if (!file) logger::warn("Combo System settings file could not be written: {}", path);
        } else {
            logger::warn("Combo System settings file could not be opened for writing: {}", path);
        }
    }

    void LoadSettings() {
        bool loadedAnySplitFile = false;
        bool shouldSaveSplitFiles = false;

        rapidjson::Document playerDoc;
        if (LoadDocumentFromFile(PLAYER_SETTINGS_PATH, playerDoc)) {
            ReadProfileSettingsFromSavedValue(playerDoc, Settings::PlayerCombo);
            loadedAnySplitFile = true;
        }

        rapidjson::Document npcDoc;
        if (LoadDocumentFromFile(NPC_SETTINGS_PATH, npcDoc)) {
            ReadProfileSettingsFromSavedValue(npcDoc, Settings::NPCCombo);
            loadedAnySplitFile = true;
        }

        rapidjson::Document uiDoc;
        if (LoadDocumentFromFile(UI_SETTINGS_PATH, uiDoc)) {
            ReadPlayerUISettings(uiDoc, Settings::PlayerUI);
            loadedAnySplitFile = true;
        }

        rapidjson::Document legacyDoc;
        if (LoadDocumentFromFile(SETTINGS_PATH, legacyDoc)) {
            if (!std::filesystem::exists(PLAYER_SETTINGS_PATH) && legacyDoc.HasMember("player")) {
                ReadProfileSettingsFromSavedValue(legacyDoc["player"], Settings::PlayerCombo);
                shouldSaveSplitFiles = true;
            }
            if (!std::filesystem::exists(NPC_SETTINGS_PATH) && legacyDoc.HasMember("npc")) {
                ReadProfileSettingsFromSavedValue(legacyDoc["npc"], Settings::NPCCombo);
                shouldSaveSplitFiles = true;
            }
            if (!std::filesystem::exists(UI_SETTINGS_PATH) && legacyDoc.HasMember("playerUI")) {
                ReadPlayerUISettings(legacyDoc["playerUI"], Settings::PlayerUI);
                shouldSaveSplitFiles = true;
            }
            if (!std::filesystem::exists(UI_SETTINGS_PATH) && legacyDoc.HasMember("showFloatingMessages") && legacyDoc["showFloatingMessages"].IsBool()) {
                Settings::PlayerUI.showFloatingMessages = legacyDoc["showFloatingMessages"].GetBool();
                shouldSaveSplitFiles = true;
            }
            loadedAnySplitFile = true;
        }

        if (!loadedAnySplitFile) {
            shouldSaveSplitFiles = true;
        }

        LoadRules();
        if (shouldSaveSplitFiles) {
            SaveSettings();
        }
    }

    void SaveSettings() {
        std::filesystem::create_directories(std::filesystem::path(UI_SETTINGS_PATH).parent_path());

        rapidjson::Document playerDoc;
        playerDoc.SetObject();
        auto& playerAlloc = playerDoc.GetAllocator();
        rapidjson::Value player(rapidjson::kObjectType);
        WriteProfileSettings(player, playerAlloc, Settings::PlayerCombo);
        playerDoc.CopyFrom(player, playerAlloc);
        WriteDocumentToFile(PLAYER_SETTINGS_PATH, playerDoc);

        rapidjson::Document npcDoc;
        npcDoc.SetObject();
        auto& npcAlloc = npcDoc.GetAllocator();
        rapidjson::Value npc(rapidjson::kObjectType);
        WriteProfileSettings(npc, npcAlloc, Settings::NPCCombo);
        npcDoc.CopyFrom(npc, npcAlloc);
        WriteDocumentToFile(NPC_SETTINGS_PATH, npcDoc);

        rapidjson::Document uiDoc;
        uiDoc.SetObject();
        auto& uiAlloc = uiDoc.GetAllocator();
        rapidjson::Value playerUI(rapidjson::kObjectType);
        WritePlayerUISettings(playerUI, uiAlloc, Settings::PlayerUI);
        uiDoc.CopyFrom(playerUI, uiAlloc);
        WriteDocumentToFile(UI_SETTINGS_PATH, uiDoc);

        for (const auto& rule : Settings::ComboRules) {
            SaveRule(rule);
        }
    }

    void PlayerRender() {
        if (RenderProfileSettings("PlayerComboSettings", Settings::PlayerCombo)) {
            SaveSettings();
        }
    }

    void UIRender() {
        if (RenderPlayerUISettings()) {
            SaveSettings();
            SKSE::GetTaskInterface()->AddTask([]() {
                Prisma::ApplyUISettings();
            });
        }
    }

    void NPCRender() {
        if (RenderProfileSettings("NPCComboSettings", Settings::NPCCombo)) {
            SaveSettings();
        }
    }

    void RulesRender() {
        bool changed = false;

        ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "%s", GetLoc("menu.combo_rules_header", "Combo Rules (By Perk)"));
        if (ImGui::Button(GetLoc("menu.add_rule", "+ Add Rule"))) {
            Settings::ComboRule newRule;
            newRule.ruleName = "New Rule " + std::to_string(Settings::ComboRules.size() + 1);
            Settings::ComboRules.push_back(newRule);
            changed = true;
        }

        ImGui::Spacing();

        for (size_t i = 0; i < Settings::ComboRules.size();) {
            auto& rule = Settings::ComboRules[i];
            ImGui::PushID(static_cast<int>(i));

            if (ImGui::CollapsingHeader(rule.ruleName.c_str())) {
                ImGui::Indent();

                char nameBuf[128];
                strcpy_s(nameBuf, rule.ruleName.c_str());
                if (ImGui::InputText(GetLoc("menu.rule_name", "Rule Name"), nameBuf, sizeof(nameBuf))) {
                    std::string newName(nameBuf);
                    if (!newName.empty() && newName != rule.ruleName) {
                        const std::string oldPath = RULES_DIR + rule.ruleName + ".json";
                        if (std::filesystem::exists(oldPath)) {
                            std::filesystem::remove(oldPath);
                        }
                        rule.ruleName = newName;
                        changed = true;
                    }
                }

                RE::FormID previousPerk = rule.perkID;
                std::string perkLabel = std::string(GetLoc("menu.target_perk", "Target Perk")) + "##" + std::to_string(i);
                if (DrawDropdown(perkLabel.c_str(), "Perk", rule.perkID, 360.0f)) {
                    bool conflict = false;
                    if (rule.perkID != 0) {
                        for (size_t j = 0; j < Settings::ComboRules.size(); j++) {
                            if (i != j && Settings::ComboRules[j].perkID == rule.perkID) {
                                conflict = true;
                                break;
                            }
                        }
                    }
                    if (conflict) {
                        rule.perkID = previousPerk;
                    } else {
                        changed = true;
                    }
                }

                ImGui::Separator();
                if (RenderProfileSettings("RuleProfile", rule.profile)) {
                    changed = true;
                }

                ImGui::Spacing();
                if (ImGui::Button(GetLoc("menu.remove_rule", "Remove Rule"), { 150, 0 })) {
                    const std::string path = RULES_DIR + rule.ruleName + ".json";
                    if (std::filesystem::exists(path)) {
                        std::filesystem::remove(path);
                    }
                    Settings::ComboRules.erase(Settings::ComboRules.begin() + i);
                    changed = true;
                    ImGui::PopID();
                    continue;
                }

                ImGui::Unindent();
            }

            ImGui::PopID();
            i++;
        }

        if (changed) {
            SaveSettings();
        }
    }

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            return;
        }

        LoadLanguage();
        LoadSettings();
        SKSEMenuFramework::SetSection("Combo System");
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.player_settings", "Player Settings"), PlayerRender);
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.ui_settings", "UI Settings"), UIRender);
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.npc_settings", "NPC Settings"), NPCRender);
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.rules_settings", "Rules Settings"), RulesRender);
    }

    void ReadJSON(rapidjson::Document& doc, const char* nome, std::vector<int>& lista) {
        if (doc.HasMember(nome) && doc[nome].IsArray()) {
            lista.clear();
            for (auto& v : doc[nome].GetArray()) {
                if (v.IsInt()) {
                    lista.push_back(v.GetInt());
                }
            }
        }
    }

    void WriteJSON(rapidjson::Document& doc, rapidjson::Document::AllocatorType& alloc, const char* nome, const std::vector<int>& lista) {
        rapidjson::Value array(rapidjson::kArrayType);
        for (int id : lista) {
            array.PushBack(id, alloc);
        }
        rapidjson::Value key;
        key.SetString(nome, alloc);
        doc.AddMember(key, array, alloc);
    }
}
