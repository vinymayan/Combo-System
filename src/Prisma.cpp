#include "Prisma.h"
#include "PrismaUI_API.h"
#include <string>
#include "Events.h"
PRISMA_UI_API::IVPrismaUI1* PrismaUI = nullptr;
static PrismaView view;

void Prisma::Install() {
    PrismaUI = reinterpret_cast<PRISMA_UI_API::IVPrismaUI1*>(PRISMA_UI_API::RequestPluginAPI());

    if (PrismaUI) {
        SKSE::log::debug("Prisma::Install - API do PrismaUI vinculada com sucesso.");
    }
    else {
        SKSE::log::error("Prisma::Install - FALHA ao obter a API do PrismaUI! O plugin PrismaUI.dll esta instalado?");
    }
}

void Prisma::Show() {
    if (!createdView) {
        createdView = true;

#ifdef DEV_SERVER
        constexpr const char* path = "http://localhost:5173";
#else
        constexpr const char* path = PRODUCT_NAME "/index.html";
#endif

        SKSE::log::debug("Prisma::Show - Criando View no endereco: {}", path);

        // 1. Cria a View e armazena o handle válido na variável 'view'
        view = PrismaUI->CreateView(path, [](PrismaView viewHandle) -> void {
            SKSE::log::debug("Prisma::Show - View pronta (DOM Ready). Removendo foco de input.");
            });

        // 2. CORREÇÃO: Regista os Listeners AGORA, pois a 'view' já possui um ID real e válido!
        if (PrismaUI && view) {
            PrismaUI->RegisterJSListener(view, "hideWindow", [](const char* data) -> void {
                PrismaUI->Hide(view);
                });

            PrismaUI->RegisterJSListener(view, "resetPlayerCombo", [](const char* data) -> void {
                SKSE::log::debug("Prisma::JSListener - Tempo de combo esgotado na UI. Agendando reset...");

                SKSE::GetTaskInterface()->AddTask([]() {
                    auto player = RE::PlayerCharacter::GetSingleton();
                    if (player) {
                        Sink::ComboManager::GetSingleton()->RemoveActor(player);
                    }
                    });
                });
            SKSE::log::debug("Prisma::Show - Listeners de JS registados com sucesso na view ativa.");
        }

        return;
    }
    PrismaUI->Show(view);
}

void Prisma::Hide() {
    if (PrismaUI && view) {
        PrismaUI->Hide(view);
    }
}

bool Prisma::IsHidden() { return PrismaUI->IsHidden(view); }

// Em Prisma.cpp
void Prisma::UpdateCombo(int hitValue, int comboValue) {
    if (!PrismaUI || !view) return;

    // CORREÇÃO: Alocação estática para impedir que a string seja destruída ao sair da função
    static std::string payload;
    payload = std::to_string(hitValue) + "|" + std::to_string(comboValue);
	logger::debug("Prisma::UpdateCombo - Enviando atualização para a UI -> Hit: {}, Combo: {}, Payload: {}", hitValue, comboValue, payload);
    try {
		logger::debug("Prisma::UpdateCombo - Enviando atualização para a UI de forma segura...");
        // Envia o ponteiro persistente e seguro de forma assíncrona
        PrismaUI->InteropCall(view, "updateComboMeter", payload.c_str());
    }
    catch (const std::exception& e) {
        SKSE::log::error("Prisma::UpdateCombo - Erro capturado na execucao: {}", e.what());
    }
    catch (...) {
        SKSE::log::error("Prisma::UpdateCombo - Erro desconhecido fatal interceptado durante o InteropCall!");
    }
}

void Prisma::SetTimerPaused(bool paused) {
    if (PrismaUI && view) {
        const char* payload = paused ? "true" : "false";
        PrismaUI->InteropCall(view, "setComboTimerPaused", payload);
    }
}