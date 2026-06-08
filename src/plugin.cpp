#include "Plugin.h"
#include "Configuration.h"
#include "Events.h"
#include "Hooks.h"
#include "Prisma.h"
#include "Manager.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kPostLoad) {
        ModMenu::Register();
        Prisma::Install();
    }

    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        auto eventHolder = RE::ScriptEventSourceHolder::GetSingleton();
        eventHolder->AddEventSink<RE::TESHitEvent>(Sink::HitEventHandler::GetSingleton());
        eventHolder->AddEventSink<RE::TESCombatEvent>(Sink::CombatEventHandler::GetSingleton());
        eventHolder->AddEventSink<RE::TESObjectLoadedEvent>(Sink::ObjectLoadedEventHandler::GetSingleton());
        auto ui = RE::UI::GetSingleton();
        if (ui) {
            ui->AddEventSink<RE::MenuOpenCloseEvent>(Sink::MenuOpenCloseEventHandler::GetSingleton());
        }

        Prisma::Preload();
        Sink::AnimationSinks::Reset();
        Sink::ComboManager::GetSingleton()->ResetAll();
        Sink::AnimationSinks::RegisterExistingActors();
        Manager::GetSingleton()->PopulateAllLists();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    SetupLog();
    Hooks::Install();
    logger::info("Plugin loaded");
    return true;
}
