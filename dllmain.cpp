// dllmain.cpp - tab registered once, ImGui enabled every UI init, loads cfg on UI init
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <imgui.h>

#include <fstream>

class WuchangDamageScaler : public RC::CppUserModBase {
public:
    static constexpr float kReset = 0.33f; // default
    static constexpr float kMin = 0.0f;      // clamp lower bound
    static constexpr const char* kCfgPath = "ue4ss/Mods/WuchangDamageScaler/dmg_mult.cfg";

    float mult = kReset;
    bool  tab_registered = false;

    static bool load_from_cfg(float& out_value) {
        if (std::ifstream f(kCfgPath); f) {
            double v{};
            if (f >> v) {
                if (v < kMin) v = kMin;
                out_value = static_cast<float>(v);
                return true;
            }
        }
        return false;
    }

    void on_ui_init() override
    {
        UE4SS_ENABLE_IMGUI();            // enable every UI init (Ctrl+O toggles)

        (void)load_from_cfg(mult);       // reflect current cfg value when GUI appears

        if (tab_registered) return;      // register only once

        register_tab(STR("Damage Scaler"), [](RC::CppUserModBase* base) {
            auto* self = static_cast<WuchangDamageScaler*>(base);

            ImGui::PushID("WuchangDS");  // namespace IDs to avoid collisions

            ImGui::TextUnformatted("Enemy damage multiplier");

            ImGui::SetNextItemWidth(140.0f);
            ImGui::InputFloat("##mult_box##WuchangDS", &self->mult, 0.0f, 0.0f, "%.2f");

            ImGui::SameLine();
            if (ImGui::Button("Save##WuchangDS")) {
                if (self->mult < kMin) self->mult = kMin;
                if (std::ofstream out(kCfgPath, std::ios::trunc | std::ios::binary); out) {
                    out << self->mult << "\n";
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset##WuchangDS")) {
                self->mult = kReset;
                if (std::ofstream out(kCfgPath, std::ios::trunc | std::ios::binary); out) {
                    out << self->mult << "\n";
                }
            }

            ImGui::PopID();
            });

        tab_registered = true;
    }
};

#define API __declspec(dllexport)
extern "C" {
    API RC::CppUserModBase* start_mod() { return new WuchangDamageScaler(); }
    API void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
}
