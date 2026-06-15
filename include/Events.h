#pragma once
#include <shared_mutex>
#include <chrono> 
#include <unordered_map> 
#include <unordered_set>
#include <string_view>

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
        int comboPoints = 0;
        RE::FormID lastSourceFormID = 0;
        int lastSourceType = 0;
        HitType lastHitType = HitType::None;
        std::chrono::steady_clock::time_point lastHitTime; // PROPRIEDADE NOVA: Guarda o timestamp preciso do último golpe
        float decayAccumulator = 0.0f;
        float decayUpdateAccumulator = 0.0f;
    };

    class ComboManager {
    public:
        static ComboManager* GetSingleton() {
            static ComboManager singleton;
            return &singleton;
        }

        // Registra o acerto feito por um atacante (aumenta combo)
        void RegisterHit(RE::FormID attackerFormID, HitType hitType, RE::FormID sourceFormID);

        // Registra o dano recebido por um alvo (diminui combo)
        void RegisterGetHit(RE::FormID targetFormID);
        void AdjustCombo(RE::FormID actorFormID, int pointsDelta);
        void ProcessAnimationEvent(RE::FormID actorFormID, std::string_view eventName);

        // Remove o actor do banco (Reset total)
        void RemoveActor(RE::FormID actorFormID);
        void ResetAll();
        void UpdateDecay(float deltaTime);
        // Identifica o tipo de hit baseado no evento
        HitType DetermineHitType(const RE::TESHitEvent* a_event);
    private:
        ComboManager() = default;

        std::unordered_map<RE::FormID, ActorComboData> _registry;
        std::shared_mutex _mutex;




        // Atualiza as variáveis diretamente no Graph do Skyrim
        void UpdateGraphVariables(RE::FormID actorFormID, const ActorComboData& data, int previousTier = -1);
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

    class AnimationEventHandler : public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    public:
        static AnimationEventHandler* GetSingleton() {
            static AnimationEventHandler singleton;
            return &singleton;
        }
        RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
            RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source) override;
    };

    class CombatEventHandler : public RE::BSTEventSink<RE::TESCombatEvent> {
    public:
        static CombatEventHandler* GetSingleton() {
            static CombatEventHandler singleton;
            return &singleton;
        }
        RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* a_event,
            RE::BSTEventSource<RE::TESCombatEvent>* a_source) override;
    };

    class ObjectLoadedEventHandler : public RE::BSTEventSink<RE::TESObjectLoadedEvent> {
    public:
        static ObjectLoadedEventHandler* GetSingleton() {
            static ObjectLoadedEventHandler singleton;
            return &singleton;
        }
        RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* a_event,
            RE::BSTEventSource<RE::TESObjectLoadedEvent>* a_source) override;
    };

    namespace AnimationSinks {
        void RegisterActor(RE::Actor* actor);
        void UnregisterActor(RE::Actor* actor);
        void RegisterExistingActors();
        void Reset();
    }
}

