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

    void ComboManager::RegisterHit(RE::Actor* attacker, const RE::TESHitEvent* a_event) {
        if (!attacker) return;

        auto formID = attacker->GetFormID();
        HitType currentHitType = DetermineHitType(a_event);
        int pointsGained = 10;
        ActorComboData dataCopy;

        auto nowTime = std::chrono::steady_clock::now();

        {
            std::unique_lock lock(_mutex);
            auto& data = _registry[formID];
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

        // OPERAÇÃO SEGURA: Registra no log apenas tipos primitivos numéricos na thread de física
        SKSE::log::debug("ComboManager::RegisterHit - Atacante FormID: {:X}, Hit: {}, Combo: {}", formID, dataCopy.hitValue, dataCopy.comboValue);
        UpdateGraphVariables(attacker, dataCopy);

        // CORREÇÃO: Captura apenas o formID (uint32_t) por valor, evitando CreateRefHandle() em background
        Utils::DelayedDispatcher::Get().PostDelayed(std::chrono::seconds(20), [this, formID, nowTime]() {
            // Executa a verificação e limpeza síncrona diretamente na Thread Principal do Skyrim
            SKSE::GetTaskInterface()->AddTask([this, formID, nowTime]() {
                auto actorPtr = RE::TESForm::LookupByID<RE::Actor>(formID);
                if (!actorPtr) return;

                bool shouldReset = false;

                {
                    std::unique_lock lock(_mutex);
                    auto it = _registry.find(formID);
                    if (it != _registry.end() && it->second.lastHitTime == nowTime) {
                        _registry.erase(it);
                        shouldReset = true;
                    }
                }

                if (shouldReset) {
                    SKSE::log::debug("ComboManager::DelayedDispatcher - Tempo esgotado! Resetando combo global de: {}", actorPtr->GetName());

                    // Modificações de animação executadas com segurança na Main Thread
                    actorPtr->SetGraphVariableInt("HitValueCMF", 0);
                    actorPtr->SetGraphVariableInt("ComboValueCMF", 0);

                    if (actorPtr->IsPlayer() || formID == 0x14) {
                        Prisma::UpdateCombo(0, 0);
                    }
                }
                });
            });
    }

    void ComboManager::RegisterGetHit(RE::Actor* target) {
        if (!target) return;

        auto formID = target->GetFormID();
        bool found = false;
        ActorComboData dataCopy;

        {
            std::unique_lock lock(_mutex);
            auto it = _registry.find(formID);
            if (it != _registry.end()) {
                auto& data = it->second;
                data.comboValue -= 15;
                if (data.comboValue < 0) data.comboValue = 0;

                dataCopy = data;
                found = true;
            }
        }

        if (found) {
            SKSE::log::debug("ComboManager::RegisterGetHit - Combo reduzido -> Alvo FormID: {:X}, Hit: {}, Combo: {}", formID, dataCopy.hitValue, dataCopy.comboValue);
            UpdateGraphVariables(target, dataCopy);
        }
    }

    void ComboManager::RemoveActor(RE::Actor* actor) {
        if (!actor) return;
        auto formID = actor->GetFormID();

        bool erased = false;
        {
            std::unique_lock lock(_mutex);
            auto it = _registry.find(formID);
            if (it != _registry.end()) {
                _registry.erase(it);
                erased = true;
            }
        }

        if (erased) {
            SKSE::log::debug("ComboManager::RemoveActor - Forçado via comando. Actor {:X} resetado.", formID);

            // Como RemoveActor já é invocado de uma Task na Main Thread através da UI, limpa diretamente
            actor->SetGraphVariableInt("HitValueCMF", 0);
            actor->SetGraphVariableInt("ComboValueCMF", 0);

            if (actor->IsPlayer() || formID == 0x14) {
                Prisma::UpdateCombo(0, 0);
            }
        }
    }

    void ComboManager::UpdateGraphVariables(RE::Actor* actor, const ActorComboData& data) {
        if (!actor) return;

        auto formID = actor->GetFormID();
        int hitVal = data.hitValue;
        int comboVal = data.comboValue;

        // Passa apenas o FormID e as propriedades numéricas para a task
        SKSE::GetTaskInterface()->AddTask([formID, hitVal, comboVal]() {
            // Realiza o Lookup de forma síncrona e isolada na Thread Principal
            auto actorPtr = RE::TESForm::LookupByID<RE::Actor>(formID);
            if (!actorPtr || actorPtr->IsDead()) return;

            // Aqui dentro estamos na Main Thread! É 100% seguro chamar actorPtr->GetName() e ler strings!
            SKSE::log::debug("ComboManager::UpdateGraphVariables - Atualizando Anim Graph para: {}, Hit: {}, Combo: {}", actorPtr->GetName(), hitVal, comboVal);

            actorPtr->SetGraphVariableInt("HitValueCMF", hitVal);
            actorPtr->SetGraphVariableInt("ComboValueCMF", comboVal);

            if (actorPtr->IsPlayer() || formID == 0x14) {
                Prisma::UpdateCombo(hitVal, comboVal);
            }
            });
    }

    RE::BSEventNotifyControl HitEventHandler::ProcessEvent(const RE::TESHitEvent* a_event,
        RE::BSTEventSource<RE::TESHitEvent>* a_source) {
        if (!a_event || !a_event->cause || !a_event->target) {
            return RE::BSEventNotifyControl::kContinue;
        }

        auto* target = a_event->target.get()->As<RE::Actor>();
        auto* attacker = a_event->cause.get()->As<RE::Actor>();

        if (attacker && !attacker->IsDead() && target && !target->IsDead()) {
            ComboManager::GetSingleton()->RegisterHit(attacker, a_event);
        }

        if (target && !target->IsDead()) {
            ComboManager::GetSingleton()->RegisterGetHit(target);
        }

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