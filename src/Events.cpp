#include "Events.h"
#include "Configuration.h"
#include "DelayedDispatcher.h"
#include "Prisma.h"
#include <algorithm>
#include <array>
#include <vector>

namespace Sink {
    namespace {
        constexpr int kMaxComboTier = 9;
        constexpr float kDecayUpdateInterval = 0.1f;

        const Settings::ComboProfileSettings& GetProfileForActor(RE::FormID actorFormID) {
            if (actorFormID == 0x14) {
                return Settings::PlayerCombo;
            }

            auto actor = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
            if (actor) {
                for (const auto& rule : Settings::ComboRules) {
                    auto perk = RE::TESForm::LookupByID<RE::BGSPerk>(rule.perkID);
                    if (perk && actor->HasPerk(perk)) {
                        return rule.profile;
                    }
                }
            }

            return Settings::NPCCombo;
        }

        bool IsComboEnabledForActor(RE::FormID actorFormID) {
            return GetProfileForActor(actorFormID).enabled;
        }

        const Settings::TierSettings& GetTierSettings(const Settings::ComboProfileSettings& profile, int tier) {
            const int clampedTier = std::clamp(tier, 0, Settings::kComboTierCount - 1);
            return profile.tiers[clampedTier];
        }

        const char* GetTierGotEventName(int tier) {
            static constexpr std::array<const char*, Settings::kComboTierCount> events{
                "TierGotFCMF",
                "TierGotECMF",
                "TierGotDCMF",
                "TierGotCCMF",
                "TierGotBCMF",
                "TierGotACMF",
                "TierGotSCMF",
                "TierGotSSCMF",
                "TierGotSSSCMF",
                "TierGotZCMF"
            };

            const int clampedTier = std::clamp(tier, 0, Settings::kComboTierCount - 1);
            return events[clampedTier];
        }

        bool CanAdvanceTier(const ActorComboData& data, const Settings::TierSettings& settings) {
            return !settings.requireMinHitsForTier || data.hitValue >= settings.minHitsForTier;
        }

        void ApplyTierProgression(ActorComboData& data, const Settings::ComboProfileSettings& profile) {
            while (data.comboValue > 0 && data.comboPoints < 0) {
                const int previousTier = data.comboValue - 1;
                const auto& previousSettings = GetTierSettings(profile, previousTier);
                data.comboPoints += std::max(1, previousSettings.pointsPerTier);
                data.comboValue = previousTier;
            }

            if (data.comboValue <= 0 && data.comboPoints < 0) {
                data.comboValue = 0;
                data.comboPoints = 0;
            }

            while (data.comboValue < kMaxComboTier) {
                const auto& settings = GetTierSettings(profile, data.comboValue);
                const int pointsPerTier = std::max(1, settings.pointsPerTier);
                if (!CanAdvanceTier(data, settings)) {
                    data.comboPoints = std::min(data.comboPoints, std::max(1, pointsPerTier - 1));
                    break;
                }
                if (data.comboPoints < pointsPerTier) {
                    break;
                }
                data.comboPoints -= pointsPerTier;
                data.comboValue += 1;
            }

            if (data.comboValue >= kMaxComboTier) {
                const auto& settings = GetTierSettings(profile, data.comboValue);
                data.comboValue = kMaxComboTier;
                data.comboPoints = std::min(data.comboPoints, std::max(1, settings.pointsPerTier));
            }

            if (data.comboValue == 0 && data.comboPoints == 0) {
                const auto lastHitTime = data.lastHitTime;
                data = {};
                data.lastHitTime = lastHitTime;
            }
        }

        int GetSourceType(RE::FormID sourceFormID) {
            auto sourceForm = RE::TESForm::LookupByID(sourceFormID);
            if (!sourceForm) {
                return 0;
            }

            if (auto weapon = sourceForm->As<RE::TESObjectWEAP>()) {
                return static_cast<int>(weapon->GetWeaponType()) + 1;
            }
            if (sourceForm->Is(RE::FormType::Spell)) {
                return 100;
            }
            if (sourceForm->Is(RE::FormType::Scroll)) {
                return 101;
            }
            return 200;
        }

        int GetAnimationEventDelta(std::string_view eventName, const Settings::TierSettings& settings) {
            if (eventName == "DodgedCMF") return settings.dodgePoints;
            if (eventName == "PerfDodgedCMF") return settings.perfectDodgePoints;
            if (eventName == "GotDodgedCMF") return settings.gotDodgedPoints;
            if (eventName == "GotPerfDodgeCMF") return settings.gotPerfectDodgedPoints;
            if (eventName == "ParriedCMF") return settings.parryPoints;
            if (eventName == "PerfParriedCMF") return settings.perfectParryPoints;
            if (eventName == "GotParriedCMF") return settings.gotParriedPoints;
            if (eventName == "GotPerfParriedCMF") return settings.gotPerfectParriedPoints;
            if (eventName == "UndodgeableHitCMF") return settings.undodgeableHitPoints;
            if (eventName == "HitByUndodgeableAtk") return settings.hitByUndodgeablePoints;
            if (eventName == "UnblockableHitCMF") return settings.unblockableHitPoints;
            if (eventName == "HitByUnblockAtk") return settings.hitByUnblockablePoints;
            if (eventName == "SBF_StaggerStart") return settings.staggerStartPoints;
            return 0;
        }

        const char* GetAnimationEventLabel(std::string_view eventName) {
            if (eventName == "DodgedCMF") return "Dodge";
            if (eventName == "PerfDodgedCMF") return "Perfect Dodge";
            if (eventName == "GotDodgedCMF") return "Dodged";
            if (eventName == "GotPerfDodgeCMF") return "Perfect Dodged";
            if (eventName == "ParriedCMF") return "Parry";
            if (eventName == "PerfParriedCMF") return "Perfect Parry";
            if (eventName == "GotParriedCMF") return "Parried";
            if (eventName == "GotPerfParriedCMF") return "Perfect Parried";
            if (eventName == "UndodgeableHitCMF") return "Undodgeable";
            if (eventName == "HitByUndodgeableAtk") return "Undodgeable Hit";
            if (eventName == "UnblockableHitCMF") return "Unblockable";
            if (eventName == "HitByUnblockAtk") return "Unblockable Hit";
            if (eventName == "SBF_StaggerStart") return "Stagger";
            return "Combo";
        }

        void ShowPlayerComboMessage(RE::FormID actorFormID, const std::string& label, int pointsDelta) {
            if (actorFormID == 0x14 && Settings::PlayerUI.showFloatingMessages && pointsDelta != 0) {
                SKSE::GetTaskInterface()->AddTask([label, pointsDelta]() {
                    Prisma::ShowComboMessage(label, pointsDelta);
                });
            }
        }

        bool IsFriendlyHit(RE::Actor* attacker, RE::Actor* target) {
            if (!attacker || !target) {
                return false;
            }

            if (attacker == target) {
                return true;
            }

            if (target->IsHostileToActor(attacker) || attacker->IsHostileToActor(target)) {
                return false;
            }

            const auto targetReaction = target->GetFactionReaction(attacker);
            const auto attackerReaction = attacker->GetFactionReaction(target);
            return targetReaction == RE::FIGHT_REACTION::kAlly || targetReaction == RE::FIGHT_REACTION::kFriend ||
                attackerReaction == RE::FIGHT_REACTION::kAlly || attackerReaction == RE::FIGHT_REACTION::kFriend ||
                ((attacker->IsPlayerRef() || attacker->IsPlayerTeammate()) &&
                    (target->IsPlayerRef() || target->IsPlayerTeammate()));
        }
    }

    HitType ComboManager::DetermineHitType(const RE::TESHitEvent* a_event) {
        if (!a_event) return HitType::None;

        if (a_event->flags.any(RE::TESHitEvent::Flag::kPowerAttack)) {
            return HitType::PowerAttack;
        }
        if (a_event->flags.any(RE::TESHitEvent::Flag::kBashAttack)) {
            return HitType::Bash;
        }

        if (auto sourceForm = RE::TESForm::LookupByID(a_event->source)) {
            if (sourceForm->Is(RE::FormType::Spell) || sourceForm->Is(RE::FormType::Scroll)) {
                return HitType::Magic;
            }
        }

        return HitType::NormalAttack;
    }

    void ComboManager::RegisterHit(RE::FormID attackerFormID, HitType currentHitType, RE::FormID sourceFormID) {
        if (!attackerFormID) return;
        if (!IsComboEnabledForActor(attackerFormID)) return;

        int pointsGained = 0;
        ActorComboData dataCopy;
        int previousTierForNotify = -1;
        auto nowTime = std::chrono::steady_clock::now();
        const auto& profile = GetProfileForActor(attackerFormID);
        const int sourceType = GetSourceType(sourceFormID);
        int expireSeconds = profile.expireComboSeconds;

        {
            std::unique_lock lock(_mutex);
            auto& data = _registry[attackerFormID];
            const auto& tierSettings = GetTierSettings(profile, data.comboValue);
            data.hitValue += 1;

            if (data.lastHitType == currentHitType) {
                pointsGained = tierSettings.repeatHitPoints;
            } else if (data.lastHitType != HitType::None) {
                pointsGained = tierSettings.variedHitPoints;
            } else {
                pointsGained = tierSettings.baseHitPoints;
            }

            if (data.lastSourceFormID != 0 && sourceFormID != 0 && data.lastSourceFormID != sourceFormID) {
                pointsGained += tierSettings.sourceChangeBonus;
            }
            if (data.lastSourceType != 0 && sourceType != 0 && data.lastSourceType != sourceType) {
                pointsGained += tierSettings.sourceTypeChangeBonus;
            }

            data.comboPoints += pointsGained;
            data.lastSourceFormID = sourceFormID;
            data.lastSourceType = sourceType;
            data.lastHitType = currentHitType;
            data.lastHitTime = nowTime;
            data.decayAccumulator = 0.0f;
            data.decayUpdateAccumulator = 0.0f;

            const int previousTier = data.comboValue;
            ApplyTierProgression(data, profile);
            dataCopy = data;
            previousTierForNotify = previousTier;
        }

        UpdateGraphVariables(attackerFormID, dataCopy, previousTierForNotify);
        ShowPlayerComboMessage(attackerFormID, "Hit", pointsGained);

        if (expireSeconds > 0) {
            Utils::DelayedDispatcher::Get().PostDelayed(std::chrono::seconds(expireSeconds), [this, attackerFormID, nowTime]() {
                SKSE::GetTaskInterface()->AddTask([this, attackerFormID, nowTime]() {
                    std::unique_lock lock(_mutex);
                    auto it = _registry.find(attackerFormID);
                    if (it != _registry.end() && it->second.lastHitTime == nowTime) {
                        lock.unlock();
                        RemoveActor(attackerFormID);
                    }
                });
            });
        }
    }

    void ComboManager::RegisterGetHit(RE::FormID targetFormID) {
        if (!targetFormID) return;
        if (!IsComboEnabledForActor(targetFormID)) return;

        bool found = false;
        ActorComboData dataCopy;
        int pointsLost = 0;
        int previousTierForNotify = -1;

        {
            std::unique_lock lock(_mutex);
            auto it = _registry.find(targetFormID);
            if (it != _registry.end() && (it->second.comboValue > 0 || it->second.comboPoints > 0)) {
                auto& data = it->second;
                const auto& profile = GetProfileForActor(targetFormID);
                const auto& tierSettings = GetTierSettings(profile, data.comboValue);
                pointsLost = tierSettings.getHitPenalty;
                data.comboPoints -= pointsLost;
                previousTierForNotify = data.comboValue;
                ApplyTierProgression(data, profile);

                dataCopy = data;
                found = true;
            }
        }

        if (found) {
            UpdateGraphVariables(targetFormID, dataCopy, previousTierForNotify);
            ShowPlayerComboMessage(targetFormID, "Hit Taken", -pointsLost);
        }
    }

    void ComboManager::AdjustCombo(RE::FormID actorFormID, int pointsDelta) {
        if (!actorFormID || pointsDelta == 0) return;
        if (!IsComboEnabledForActor(actorFormID)) return;

        ActorComboData dataCopy;
        bool changed = false;
        int previousTierForNotify = -1;
        auto nowTime = std::chrono::steady_clock::now();
        const auto& profile = GetProfileForActor(actorFormID);
        const int expireSeconds = profile.expireComboSeconds;

        {
            std::unique_lock lock(_mutex);
            if (pointsDelta < 0 && !_registry.contains(actorFormID)) return;
            auto& data = _registry[actorFormID];
            data.comboPoints += pointsDelta;
            data.lastHitTime = nowTime;
            data.decayAccumulator = 0.0f;
            data.decayUpdateAccumulator = 0.0f;
            previousTierForNotify = data.comboValue;
            ApplyTierProgression(data, profile);
            dataCopy = data;
            changed = true;
        }

        if (changed) {
            UpdateGraphVariables(actorFormID, dataCopy, previousTierForNotify);
        }

        if (expireSeconds > 0) {
            Utils::DelayedDispatcher::Get().PostDelayed(std::chrono::seconds(expireSeconds), [this, actorFormID, nowTime]() {
                SKSE::GetTaskInterface()->AddTask([this, actorFormID, nowTime]() {
                    std::unique_lock lock(_mutex);
                    auto it = _registry.find(actorFormID);
                    if (it != _registry.end() && it->second.lastHitTime == nowTime) {
                        lock.unlock();
                        RemoveActor(actorFormID);
                    }
                });
            });
        }
    }

    void ComboManager::ProcessAnimationEvent(RE::FormID actorFormID, std::string_view eventName) {
        if (!actorFormID || !IsComboEnabledForActor(actorFormID)) return;

        int currentTier = 0;
        {
            std::shared_lock lock(_mutex);
            auto it = _registry.find(actorFormID);
            if (it != _registry.end()) {
                currentTier = it->second.comboValue;
            }
        }

        const auto& profile = GetProfileForActor(actorFormID);
        const int pointsDelta = GetAnimationEventDelta(eventName, GetTierSettings(profile, currentTier));
        if (pointsDelta != 0) {
            AdjustCombo(actorFormID, pointsDelta);
            ShowPlayerComboMessage(actorFormID, GetAnimationEventLabel(eventName), pointsDelta);
        }
    }

    void ComboManager::RemoveActor(RE::FormID actorFormID) {
        if (!actorFormID) return;

        bool erased = false;
        {
            std::unique_lock lock(_mutex);
            auto it = _registry.find(actorFormID);
            if (it != _registry.end()) {
                _registry.erase(it);
                erased = true;
            }
        }

        if (erased) {
            auto actor = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
            if (!actor || actor->IsDead() || !actor->Is3DLoaded()) {
                if (actorFormID == 0x14) {
                    Prisma::UpdateCombo(0, 0, 0, GetTierSettings(Settings::PlayerCombo, 0).pointsPerTier);
                }
                return;
            }

            actor->SetGraphVariableInt("HitValueCMF", 0);
            actor->SetGraphVariableInt("ComboValueCMF", 0);
            actor->SetGraphVariableInt("ComboPointsCMF", 0);
            actor->SetGraphVariableInt("TierComboPointsCMF", 0);

            if (actor->IsPlayerRef() || actorFormID == 0x14) {
                Prisma::UpdateCombo(0, 0, 0, GetTierSettings(Settings::PlayerCombo, 0).pointsPerTier);
            }
        }
    }

    void ComboManager::ResetAll() {
        {
            std::unique_lock lock(_mutex);
            _registry.clear();
        }

        if (auto player = RE::PlayerCharacter::GetSingleton()) {
            player->SetGraphVariableInt("HitValueCMF", 0);
            player->SetGraphVariableInt("ComboValueCMF", 0);
            player->SetGraphVariableInt("ComboPointsCMF", 0);
            player->SetGraphVariableInt("TierComboPointsCMF", 0);
        }

        if (auto processLists = RE::ProcessLists::GetSingleton()) {
            for (auto& actorHandle : processLists->highActorHandles) {
                if (auto actor = actorHandle.get().get()) {
                    actor->SetGraphVariableInt("HitValueCMF", 0);
                    actor->SetGraphVariableInt("ComboValueCMF", 0);
                    actor->SetGraphVariableInt("ComboPointsCMF", 0);
                    actor->SetGraphVariableInt("TierComboPointsCMF", 0);
                }
            }
        }

        Prisma::UpdateCombo(0, 0, 0, GetTierSettings(Settings::PlayerCombo, 0).pointsPerTier);
        Prisma::Hide();
    }

    void ComboManager::UpdateDecay(float deltaTime) {
        if (deltaTime <= 0.0f) return;

        struct ChangedActor {
            RE::FormID actorFormID;
            ActorComboData data;
            int previousTier;
        };
        std::vector<ChangedActor> changedActors;

        {
            std::unique_lock lock(_mutex);
            for (auto& [actorFormID, data] : _registry) {
                const auto& profile = GetProfileForActor(actorFormID);
                const auto& settings = GetTierSettings(profile, data.comboValue);
                if (!settings.losePointsPerSecond || settings.pointsLostPerSecond <= 0 || (data.comboValue <= 0 && data.comboPoints <= 0)) {
                    continue;
                }

                data.decayAccumulator += deltaTime * static_cast<float>(settings.pointsLostPerSecond);
                data.decayUpdateAccumulator += deltaTime;
                if (data.decayUpdateAccumulator < kDecayUpdateInterval) {
                    continue;
                }

                data.decayUpdateAccumulator = 0.0f;
                const int pointsToLose = static_cast<int>(data.decayAccumulator);
                if (pointsToLose <= 0) {
                    continue;
                }

                data.decayAccumulator -= static_cast<float>(pointsToLose);
                data.comboPoints -= pointsToLose;
                const int previousTier = data.comboValue;
                ApplyTierProgression(data, profile);
                changedActors.push_back({ actorFormID, data, previousTier });
            }
        }

        for (const auto& changedActor : changedActors) {
            UpdateGraphVariables(changedActor.actorFormID, changedActor.data, changedActor.previousTier);
        }
    }

    void ComboManager::UpdateGraphVariables(RE::FormID actorFormID, const ActorComboData& data, int previousTier) {
        if (!actorFormID) return;

        const int hitVal = data.hitValue;
        const int comboVal = data.comboValue;
        const int comboPoints = data.comboPoints;
        const auto& profile = GetProfileForActor(actorFormID);
        const int pointsPerTier = std::max(1, GetTierSettings(profile, comboVal).pointsPerTier);
        int totalComboPoints = comboPoints;
        for (int tier = 0; tier < comboVal; tier++) {
            totalComboPoints += std::max(1, GetTierSettings(profile, tier).pointsPerTier);
        }
        const int clampedPreviousTier = previousTier >= 0 ? std::clamp(previousTier, 0, Settings::kComboTierCount - 1) : comboVal;
        const bool tierChanged = previousTier >= 0 && clampedPreviousTier != comboVal;
        const bool tierAdvanced = tierChanged && comboVal > clampedPreviousTier;


            auto actorPtr = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
            if (!actorPtr || actorPtr->IsDead() || !actorPtr->Is3DLoaded()) return;

            actorPtr->SetGraphVariableInt("HitValueCMF", hitVal);
            actorPtr->SetGraphVariableInt("ComboValueCMF", comboVal);
            actorPtr->SetGraphVariableInt("ComboPointsCMF", totalComboPoints);
            actorPtr->SetGraphVariableInt("TierComboPointsCMF", comboPoints);

            if (tierChanged) {
                actorPtr->NotifyAnimationGraph(tierAdvanced ? "TierAdvanceCMF" : "TierReduceCMF");
                actorPtr->NotifyAnimationGraph(GetTierGotEventName(comboVal));
            }

            if (actorPtr->IsPlayerRef() || actorFormID == 0x14) {
                SKSE::GetTaskInterface()->AddTask([hitVal, comboVal, comboPoints, pointsPerTier, totalComboPoints]() {
                    Prisma::UpdateCombo(hitVal, comboVal, comboPoints, pointsPerTier, totalComboPoints);
                });
            }

    }

    RE::BSEventNotifyControl HitEventHandler::ProcessEvent(
        const RE::TESHitEvent* a_event,
        RE::BSTEventSource<RE::TESHitEvent>* a_source) {
        if (!a_event || !a_event->cause || !a_event->target) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto targetRef = a_event->target.get();
        if (!targetRef) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* target = targetRef->As<RE::Actor>();
        if (!target || target->IsDead()) {
            return RE::BSEventNotifyControl::kContinue;
        }

        const auto targetFormID = target->GetFormID();
        RE::FormID attackerFormID = 0;
        RE::Actor* attackerActor = nullptr;
        if (auto causeRef = a_event->cause.get()) {
            if (auto* attacker = causeRef->As<RE::Actor>()) {
                attackerActor = attacker;
                attackerFormID = attacker->GetFormID();
                SKSE::log::debug("Hit fired by actor: FormID [0x{:08X}], Name: '{}'", attackerFormID, attacker->GetName());
            }
        }

        if (IsFriendlyHit(attackerActor, target)) {
            SKSE::log::debug(
                "Ignoring friendly combo hit: attacker [0x{:08X}] '{}' -> target [0x{:08X}] '{}'",
                attackerFormID,
                attackerActor ? attackerActor->GetName() : "",
                targetFormID,
                target->GetName());
            return RE::BSEventNotifyControl::kContinue;
        }

        const auto hitType = ComboManager::GetSingleton()->DetermineHitType(a_event);

        if (attackerFormID) {
            ComboManager::GetSingleton()->RegisterHit(attackerFormID, hitType, a_event->source);
        }

        ComboManager::GetSingleton()->RegisterGetHit(targetFormID);

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl MenuOpenCloseEventHandler::ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) {
        if (a_event) {
            auto ui = RE::UI::GetSingleton();
            if (ui) {
                const bool isGamePaused = ui->GameIsPaused();

                Prisma::SetTimerPaused(isGamePaused);

                if (isGamePaused) {
                    Utils::DelayedDispatcher::Get().Pause();
                } else {
                    Utils::DelayedDispatcher::Get().Resume();
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl AnimationEventHandler::ProcessEvent(
        const RE::BSAnimationGraphEvent* a_event,
        RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source) {
        if (!a_event || !a_event->holder) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto actor = a_event->holder->As<RE::Actor>();
        if (!actor || actor->IsDead()) {
            return RE::BSEventNotifyControl::kContinue;
        }

        const auto actorFormID = actor->GetFormID();
        if (!IsComboEnabledForActor(actorFormID)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        ComboManager::GetSingleton()->ProcessAnimationEvent(actorFormID, a_event->tag);

        return RE::BSEventNotifyControl::kContinue;
    }

    namespace AnimationSinks {
        static std::shared_mutex g_sinkMutex;
        static std::unordered_set<RE::FormID> g_registeredActors;

        void RegisterActor(RE::Actor* actor) {
            if (!actor || actor->IsDead()) return;

            std::unique_lock lock(g_sinkMutex);
            if (g_registeredActors.insert(actor->GetFormID()).second) {
                actor->AddAnimationGraphEventSink(AnimationEventHandler::GetSingleton());
            }
        }

        void UnregisterActor(RE::Actor* actor) {
            if (!actor) return;

            std::unique_lock lock(g_sinkMutex);
            if (g_registeredActors.erase(actor->GetFormID()) > 0) {
                actor->RemoveAnimationGraphEventSink(AnimationEventHandler::GetSingleton());
            }
        }

        void RegisterExistingActors() {
            if (auto player = RE::PlayerCharacter::GetSingleton()) {
                RegisterActor(player);
            }

            if (auto processLists = RE::ProcessLists::GetSingleton()) {
                for (auto& actorHandle : processLists->highActorHandles) {
                    if (auto actor = actorHandle.get().get()) {
                        RegisterActor(actor);
                    }
                }
            }
        }

        void Reset() {
            std::unique_lock lock(g_sinkMutex);
            g_registeredActors.clear();
        }
    }

    RE::BSEventNotifyControl CombatEventHandler::ProcessEvent(
        const RE::TESCombatEvent* a_event,
        RE::BSTEventSource<RE::TESCombatEvent>* a_source) {
        if (!a_event || !a_event->actor) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto ref = a_event->actor.get();
        auto actor = ref ? ref->As<RE::Actor>() : nullptr;
        if (!actor) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (a_event->newState.get() == RE::ACTOR_COMBAT_STATE::kCombat) {
            AnimationSinks::RegisterActor(actor);
        } else if (a_event->newState.get() == RE::ACTOR_COMBAT_STATE::kNone && !actor->IsPlayerRef()) {
            AnimationSinks::UnregisterActor(actor);
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ObjectLoadedEventHandler::ProcessEvent(
        const RE::TESObjectLoadedEvent* a_event,
        RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source) {
        if (!a_event || !a_event->loaded) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto form = RE::TESForm::LookupByID(a_event->formID);
        auto actor = form ? form->As<RE::Actor>() : nullptr;
        if (actor) {
            AnimationSinks::RegisterActor(actor);
        }

        return RE::BSEventNotifyControl::kContinue;
    }
}
