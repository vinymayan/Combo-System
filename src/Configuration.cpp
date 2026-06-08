#include "Configuration.h"
#include "Manager.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <cstring>
#include <filesystem>

namespace ImGui = ImGuiMCP;

namespace ModMenu {
    constexpr const char* SETTINGS_PATH = "Data/SKSE/Plugins/ComboCount/Settings.json";
    constexpr const char* LANG_PATH = "Data/SKSE/Plugins/ComboCount/Language.json";
    const std::string RULES_DIR = "Data/SKSE/Plugins/ComboCount/Rules/";
    static std::unordered_map<std::string, std::string> LangMap;

    void LoadLanguage() {
        LangMap.clear();

        std::ifstream file(LANG_PATH, std::ios::binary);
        if (!file.is_open()) {
            logger::warn("Combo Count language file not found. Falling back to default text.");
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
        ImGui::SetNextItemWidth(120.0f);
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

        if (RenderIntSliderWithInput(GetLoc("menu.points_per_tier", "Combo points required to advance tier"), &settings.pointsPerTier, 1, 2000)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.base_hit_points", "Base hit points"), &settings.baseHitPoints, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.repeat_hit_points", "Repeated hit points"), &settings.repeatHitPoints, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.varied_hit_points", "Varied hit points"), &settings.variedHitPoints, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.get_hit_penalty", "Penalty when actor is hit"), &settings.getHitPenalty, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.source_change_bonus", "Bonus when weapon/magic changes"), &settings.sourceChangeBonus, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.source_type_change_bonus", "Bonus when weapon/magic type changes"), &settings.sourceTypeChangeBonus, 0, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.dodge_points", "Dodge event points"), &settings.dodgePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.perfect_dodge_points", "Perfect dodge event points"), &settings.perfectDodgePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_dodged_points", "Got dodged event points"), &settings.gotDodgedPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_perfect_dodged_points", "Got perfect dodged event points"), &settings.gotPerfectDodgedPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.parry_points", "Parry event points"), &settings.parryPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.perfect_parry_points", "Perfect parry event points"), &settings.perfectParryPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_parried_points", "Got parried event points"), &settings.gotParriedPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.got_perfect_parried_points", "Got perfect parried event points"), &settings.gotPerfectParriedPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.undodgeable_hit_points", "Undodgeable hit event points"), &settings.undodgeableHitPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.hit_by_undodgeable_points", "Hit by undodgeable event points"), &settings.hitByUndodgeablePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.unblockable_hit_points", "Unblockable hit event points"), &settings.unblockableHitPoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.hit_by_unblockable_points", "Hit by unblockable event points"), &settings.hitByUnblockablePoints, -500, 500)) changed = true;
        if (RenderIntSliderWithInput(GetLoc("menu.stagger_start_points", "Stagger start event points"), &settings.staggerStartPoints, -500, 500)) changed = true;

        if (ImGui::Checkbox(GetLoc("menu.lose_points_per_second", "Lose combo points per second"), &settings.losePointsPerSecond)) changed = true;
        if (settings.losePointsPerSecond) {
            if (RenderIntSliderWithInput(GetLoc("menu.points_lost_per_second", "Combo points lost per second"), &settings.pointsLostPerSecond, 0, 500)) changed = true;
        }

        if (ImGui::Checkbox(GetLoc("menu.require_min_hits_for_tier", "Require a minimum hit count to advance tier"), &settings.requireMinHitsForTier)) changed = true;
        if (settings.requireMinHitsForTier) {
            if (RenderIntSliderWithInput(GetLoc("menu.min_hits_for_tier", "Minimum hits required to advance tier"), &settings.minHitsForTier, 1, 500)) changed = true;
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

    void LoadSettings() {
        std::ifstream file(SETTINGS_PATH, std::ios::binary);
        if (!file.is_open()) {
            SaveSettings();
            LoadRules();
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
            logger::warn("Combo Count settings could not be parsed. Using defaults.");
            LoadRules();
            return;
        }

        if (doc.HasMember("player")) {
            ReadProfileSettingsFromSavedValue(doc["player"], Settings::PlayerCombo);
        }
        if (doc.HasMember("npc")) {
            ReadProfileSettingsFromSavedValue(doc["npc"], Settings::NPCCombo);
        }

        LoadRules();
    }

    void SaveSettings() {
        std::filesystem::create_directories("Data/SKSE/Plugins/ComboCount");

        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        rapidjson::Value player(rapidjson::kObjectType);
        WriteProfileSettings(player, alloc, Settings::PlayerCombo);
        doc.AddMember("player", player, alloc);

        rapidjson::Value npc(rapidjson::kObjectType);
        WriteProfileSettings(npc, alloc, Settings::NPCCombo);
        doc.AddMember("npc", npc, alloc);

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::ofstream file(SETTINGS_PATH, std::ios::binary);
        if (file.is_open()) {
            file << buffer.GetString();
        }

        for (const auto& rule : Settings::ComboRules) {
            SaveRule(rule);
        }
    }

    void PlayerRender() {
        if (RenderProfileSettings("PlayerComboSettings", Settings::PlayerCombo)) {
            SaveSettings();
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
        SKSEMenuFramework::SetSection("Combo Count");
        SKSEMenuFramework::AddSectionItem(GetLoc("menu.player_settings", "Player Settings"), PlayerRender);
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
