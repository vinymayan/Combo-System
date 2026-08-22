#include "Plugin.h"
#include "Configuration.h"
#include "Events.h"
#include "Hooks.h"
#include "Prisma.h"
#include "Manager.h"
namespace {
    bool hasDFG = false;

    class DynamicFormsGeneratorListener : public RE::BSTEventSink<SKSE::ModCallbackEvent> {
    public:
        static DynamicFormsGeneratorListener* GetSingleton()
        {
            static DynamicFormsGeneratorListener singleton;
            return &singleton;
        }

        void Register()
        {
            if (auto dispatcher = SKSE::GetModCallbackEventSource()) {
                dispatcher->AddEventSink(this);
            }
        }

        RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override
        {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            std::string_view eventName = a_event->eventName.c_str();
            if (eventName == "DynamicFormsGeneratorLoaded") {
                Manager::GetSingleton()->PopulateAllLists();
                return RE::BSEventNotifyControl::kContinue;
            }
            if (eventName == "DynamicFormsGeneratorUpdated") {
                Manager::GetSingleton()->RefreshLists(a_event->strArg.c_str());
                return RE::BSEventNotifyControl::kContinue;
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };
}

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kPostLoad) {
        hasDFG = GetModuleHandleA("DynamicFormsGenerator.dll") != nullptr;
        if (hasDFG) {
            logger::info("DynamicFormsGenerator.dll found");
        }
        ModMenu::Register();
        Prisma::Install();
    }

    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        if (!hasDFG) {
            Manager::GetSingleton()->PopulateAllLists();
        }
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
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    SetupLog();
    Hooks::Install();
    DynamicFormsGeneratorListener::GetSingleton()->Register();
    logger::info("Plugin loaded");
    return true;
}
