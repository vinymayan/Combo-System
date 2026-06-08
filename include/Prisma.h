#pragma once
#include <string>

class Prisma {
    static inline bool createdView = false;
public:
    static void Install();
    static void Preload();
    static void Show();
    static void Hide();
    static bool IsHidden();
    static void UpdateCombo(int hitValue, int comboValue, int comboPoints = 0, int pointsPerTier = 100);
    static void ShowComboMessage(const std::string& label, int pointsDelta);
    static void ApplyUISettings();
    static void ResetComboDisplay();
    static void SetTimerPaused(bool paused);
};
