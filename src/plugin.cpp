#include "Plugin.h"
#include "Hooks.h"
#include "Prisma.h"
#include "Events.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kPostLoad) {
        Prisma::Install();
        
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESHitEvent>(Sink::HitEventHandler::GetSingleton());
        auto ui = RE::UI::GetSingleton();
        if (ui) {
            ui->AddEventSink<RE::MenuOpenCloseEvent>(Sink::MenuOpenCloseEventHandler::GetSingleton());
        }
        Prisma::Show();
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            player->SetGraphVariableInt("HitValueCMF", 0);
            player->SetGraphVariableInt("ComboValueCMF", 0);
        }
        if (auto processLists = RE::ProcessLists::GetSingleton()) {
            for (auto& actorHandle : processLists->highActorHandles) {
                if (auto actor = actorHandle.get().get()) {
                    actor->SetGraphVariableInt("HitValueCMF", 0);
                    actor->SetGraphVariableInt("ComboValueCMF", 0);
                }
            }
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    SetupLog();
    logger::info("Plugin loaded");
    return true;
}