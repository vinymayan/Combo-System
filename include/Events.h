#pragma once
#include <shared_mutex>
#include <chrono> 
#include <unordered_map> 
#include <unordered_set>

namespace Sink {
    enum class HitType {
        None,
        NormalAttack,
        PowerAttack,
        Magic,
        Bash
    };

    struct ActorComboData {
        int hitValue = 0;
        int comboValue = 0;
        HitType lastHitType = HitType::None;
        std::chrono::steady_clock::time_point lastHitTime; // PROPRIEDADE NOVA: Guarda o timestamp preciso do último golpe
    };

    class ComboManager {
    public:
        static ComboManager* GetSingleton() {
            static ComboManager singleton;
            return &singleton;
        }

        // Registra o acerto feito por um atacante (aumenta combo)
        void RegisterHit(RE::Actor* attacker, const RE::TESHitEvent* a_event);

        // Registra o dano recebido por um alvo (diminui combo)
        void RegisterGetHit(RE::Actor* target);

        // Remove o actor do banco (Reset total)
        void RemoveActor(RE::Actor* actor);

    private:
        ComboManager() = default;

        std::unordered_map<RE::FormID, ActorComboData> _registry;
        std::shared_mutex _mutex;

        // Identifica o tipo de hit baseado no evento
        HitType DetermineHitType(const RE::TESHitEvent* a_event);

        // Atualiza as variáveis diretamente no Graph do Skyrim
        void UpdateGraphVariables(RE::Actor* actor, const ActorComboData& data);
    };

    class HitEventHandler : public RE::BSTEventSink<RE::TESHitEvent> {
    public:
        static HitEventHandler* GetSingleton() {
            static HitEventHandler singleton;
            return &singleton;
        }
        RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event,
                                              RE::BSTEventSource<RE::TESHitEvent>* a_source) override;
    };

    class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MenuOpenCloseEventHandler* GetSingleton() {
            static MenuOpenCloseEventHandler singleton;
            return &singleton;
        }
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;
    };
}

