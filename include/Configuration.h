#pragma once
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <map>

namespace Settings {

}

namespace ModMenu {
    void Register();
    void PlayerRender();
    void NPCRender();
    void LoadSettings();
    void SaveSettings();
    // Utilitários de JSON
    void ReadJSON(rapidjson::Document& doc, const char* nome, std::vector<int>& lista);
    void WriteJSON(rapidjson::Document& doc, rapidjson::Document::AllocatorType& alloc, const char* nome, const std::vector<int>& lista);
}