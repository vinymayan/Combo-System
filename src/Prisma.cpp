#include "Prisma.h"
#include "PrismaUI_API.h"
#include "Configuration.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <string>
#include "Events.h"
PRISMA_UI_API::IVPrismaUI1* PrismaUI = nullptr;
static PrismaView view;

namespace {
    std::string BuildUISettingsPayload() {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        writer.StartObject();
        writer.Key("enabled");
        writer.Bool(Settings::PlayerUI.enabled);
        writer.Key("showFloatingMessages");
        writer.Bool(Settings::PlayerUI.showFloatingMessages);
        writer.Key("editMode");
        writer.Bool(Settings::PlayerUI.editMode);
        writer.Key("useTierImages");
        writer.Bool(Settings::PlayerUI.useTierImages);
        writer.Key("useTextProgressFill");
        writer.Bool(Settings::PlayerUI.useTextProgressFill);
        writer.Key("showTierName");
        writer.Bool(Settings::PlayerUI.showTierName);
        writer.Key("positionXPercent");
        writer.Int(Settings::PlayerUI.positionXPercent);
        writer.Key("positionYPercent");
        writer.Int(Settings::PlayerUI.positionYPercent);
        writer.Key("scalePercent");
        writer.Int(Settings::PlayerUI.scalePercent);
        writer.Key("progressBarWidth");
        writer.Int(Settings::PlayerUI.progressBarWidth);
        writer.Key("progressBarHeight");
        writer.Int(Settings::PlayerUI.progressBarHeight);
        writer.Key("tiers");
        writer.StartArray();
        for (int i = 0; i < Settings::kComboTierCount; i++) {
            writer.StartObject();
            writer.Key("letterColor");
            writer.StartArray();
            writer.Double(Settings::PlayerUI.tiers[i].letterColor[0]);
            writer.Double(Settings::PlayerUI.tiers[i].letterColor[1]);
            writer.Double(Settings::PlayerUI.tiers[i].letterColor[2]);
            writer.Double(Settings::PlayerUI.tiers[i].letterColor[3]);
            writer.EndArray();
            writer.Key("backgroundColor");
            writer.StartArray();
            writer.Double(Settings::PlayerUI.tiers[i].backgroundColor[0]);
            writer.Double(Settings::PlayerUI.tiers[i].backgroundColor[1]);
            writer.Double(Settings::PlayerUI.tiers[i].backgroundColor[2]);
            writer.Double(Settings::PlayerUI.tiers[i].backgroundColor[3]);
            writer.EndArray();
            writer.Key("imagePath");
            writer.String(Settings::PlayerUI.tiers[i].imagePath.c_str());
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        return buffer.GetString();
    }

    void SendUISettingsToPrisma() {
        if (!PrismaUI || !view) {
            return;
        }

        static std::string payload;
        payload = BuildUISettingsPayload();
        PrismaUI->InteropCall(view, "updateComboUiSettings", payload.c_str());
    }
}

void Prisma::Install() {
    PrismaUI = reinterpret_cast<PRISMA_UI_API::IVPrismaUI1*>(PRISMA_UI_API::RequestPluginAPI());

    if (PrismaUI) {
        SKSE::log::debug("Prisma::Install - API do PrismaUI vinculada com sucesso.");
    }
    else {
        SKSE::log::error("Prisma::Install - FALHA ao obter a API do PrismaUI! O plugin PrismaUI.dll esta instalado?");
    }
}

void Prisma::Preload() {
    Show();
    ApplyUISettings();
    ResetComboDisplay();
    Hide();
}

void Prisma::Show() {
    if (!PrismaUI) return;

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
                        Sink::ComboManager::GetSingleton()->RemoveActor(player->GetFormID());
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

bool Prisma::IsHidden() { return !PrismaUI || !view || PrismaUI->IsHidden(view); }

void Prisma::UpdateCombo(int hitValue, int comboValue, int comboPoints, int pointsPerTier) {
    if (!PrismaUI || !view) return;
    if (!Settings::PlayerUI.enabled) {
        if (!PrismaUI->IsHidden(view)) {
            PrismaUI->Hide(view);
        }
        return;
    }

    if (hitValue > 0 && PrismaUI->IsHidden(view)) {
        PrismaUI->Show(view);
    }

    static std::string payload;
    payload = std::to_string(hitValue) + "|" + std::to_string(comboValue) + "|" + std::to_string(comboPoints) + "|" + std::to_string(pointsPerTier);
	logger::debug("Prisma::UpdateCombo - Enviando atualização para a UI -> Hit: {}, Tier: {}, Points: {}, Required: {}, Payload: {}", hitValue, comboValue, comboPoints, pointsPerTier, payload);
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

void Prisma::ShowComboMessage(const std::string& label, int pointsDelta) {
    if (!PrismaUI || !view || pointsDelta == 0) return;
    if (!Settings::PlayerUI.enabled) return;
    if (!Settings::PlayerUI.showFloatingMessages) return;

    if (PrismaUI->IsHidden(view)) {
        PrismaUI->Show(view);
    }

    static std::string payload;
    payload = label + "|" + std::to_string(pointsDelta);
    PrismaUI->InteropCall(view, "showComboMessage", payload.c_str());
}

void Prisma::ApplyUISettings() {
    if (!PrismaUI) {
        return;
    }

    if (!Settings::PlayerUI.enabled) {
        if (view && !PrismaUI->IsHidden(view)) {
            PrismaUI->Hide(view);
        }
        return;
    }

    if (!createdView || !view) {
        if (Settings::PlayerUI.editMode) {
            Show();
        } else {
            return;
        }
    }

    if (Settings::PlayerUI.editMode && PrismaUI->IsHidden(view)) {
        PrismaUI->Show(view);
    }

    SendUISettingsToPrisma();
}

void Prisma::ResetComboDisplay() {
    UpdateCombo(0, 0, 0, 100);
}

void Prisma::SetTimerPaused(bool paused) {
    if (PrismaUI && view) {
        const char* payload = paused ? "true" : "false";
        PrismaUI->InteropCall(view, "setComboTimerPaused", payload);
    }
}
