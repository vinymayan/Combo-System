#include "Events.h"
#include "DelayedDispatcher.h"
#include "Prisma.h"

namespace Sink {
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

    void ComboManager::RegisterHit(RE::FormID attackerFormID, HitType currentHitType) {
        if (!attackerFormID) return;

        int pointsGained = 10;
        ActorComboData dataCopy;

        auto nowTime = std::chrono::steady_clock::now();

        {
            std::unique_lock lock(_mutex);
            auto& data = _registry[attackerFormID];
            data.hitValue += 1;

            if (data.lastHitType == currentHitType) {
                pointsGained = 2;
            }
            else if (data.lastHitType != HitType::None) {
                pointsGained = 20;
            }

            data.comboValue += pointsGained;
            data.lastHitType = currentHitType;
            data.lastHitTime = nowTime;

            dataCopy = data;
        }

        UpdateGraphVariables(attackerFormID, dataCopy);

        Utils::DelayedDispatcher::Get().PostDelayed(std::chrono::seconds(20), [this, attackerFormID, nowTime]() {
            // Executa a verificação e limpeza síncrona diretamente na Thread Principal do Skyrim
            SKSE::GetTaskInterface()->AddTask([this, attackerFormID, nowTime]() {
                auto actorPtr = RE::TESForm::LookupByID<RE::Actor>(attackerFormID);
                if (!actorPtr || actorPtr->IsDead() || !actorPtr->Is3DLoaded()) {
                    if (attackerFormID == 0x14) {
                        Prisma::UpdateCombo(0, 0);
                    }
                    return;
                }

                bool shouldReset = false;

                {
                    std::unique_lock lock(_mutex);
                    auto it = _registry.find(attackerFormID);
                    if (it != _registry.end() && it->second.lastHitTime == nowTime) {
                        _registry.erase(it);
                        shouldReset = true;
                    }
                }

                if (shouldReset) {
                    actorPtr->SetGraphVariableInt("HitValueCMF", 0);
                    actorPtr->SetGraphVariableInt("ComboValueCMF", 0);

                    if (actorPtr->IsPlayerRef() || attackerFormID == 0x14) {
                        Prisma::UpdateCombo(0, 0);
                    }
                }
                });
            });
    }

    void ComboManager::RegisterGetHit(RE::FormID targetFormID) {
        if (!targetFormID) return;

        bool found = false;
        ActorComboData dataCopy;

        {
            std::unique_lock lock(_mutex);
            auto it = _registry.find(targetFormID);
            if (it != _registry.end()) {
                auto& data = it->second;
                data.comboValue -= 15;
                if (data.comboValue < 0) data.comboValue = 0;

                dataCopy = data;
                found = true;
            }
        }

        if (found) {
            UpdateGraphVariables(targetFormID, dataCopy);
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
                    Prisma::UpdateCombo(0, 0);
                }
                return;
            }

            actor->SetGraphVariableInt("HitValueCMF", 0);
            actor->SetGraphVariableInt("ComboValueCMF", 0);

            if (actor->IsPlayerRef() || actorFormID == 0x14) {
                Prisma::UpdateCombo(0, 0);
            }
        }
    }

    void ComboManager::UpdateGraphVariables(RE::FormID actorFormID, const ActorComboData& data) {
        if (!actorFormID) return;

        int hitVal = data.hitValue;
        int comboVal = data.comboValue;

        // Passa apenas o FormID e as propriedades numéricas para a task
        SKSE::GetTaskInterface()->AddTask([actorFormID, hitVal, comboVal]() {
            // Realiza o Lookup de forma síncrona e isolada na Thread Principal
            auto actorPtr = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
            if (!actorPtr || actorPtr->IsDead() || !actorPtr->Is3DLoaded()) return;

            actorPtr->SetGraphVariableInt("HitValueCMF", hitVal);
            actorPtr->SetGraphVariableInt("ComboValueCMF", comboVal);

            if (actorPtr->IsPlayerRef() || actorFormID == 0x14) {
                Prisma::UpdateCombo(hitVal, comboVal);
            }
            });
    }

    RE::BSEventNotifyControl HitEventHandler::ProcessEvent(const RE::TESHitEvent* a_event,
        RE::BSTEventSource<RE::TESHitEvent>* a_source) {
        if (!a_event || !a_event->cause || !a_event->target) {
            return RE::BSEventNotifyControl::kContinue;
        }
        
        auto targetRef = a_event->target.get();
        if (!targetRef) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* target = targetRef->As<RE::Actor>();
        if (!target) {
            return RE::BSEventNotifyControl::kContinue;
        }


        const auto targetFormID = target->GetFormID();
        RE::FormID attackerFormID = 0;
        if (auto causeRef = a_event->cause.get()) {
            if (auto* attacker = causeRef->As<RE::Actor>()) {
                attackerFormID = attacker->GetFormID();
                SKSE::log::debug("Hit disparado por actor: FormID [0x{:08X}], Nome: '{}'", attackerFormID, attacker->GetName());
            }
        }
        const auto hitType = ComboManager::GetSingleton()->DetermineHitType(a_event);

        if (attackerFormID) {
            ComboManager::GetSingleton()->RegisterHit(attackerFormID, hitType);
        }

        ComboManager::GetSingleton()->RegisterGetHit(targetFormID);

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) {
        if (a_event) {
            auto ui = RE::UI::GetSingleton();
            if (ui) {
                bool isGamePaused = ui->GameIsPaused();

                Prisma::SetTimerPaused(isGamePaused);

                if (isGamePaused) {
                    Utils::DelayedDispatcher::Get().Pause();
                }
                else {
                    Utils::DelayedDispatcher::Get().Resume();
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
}
