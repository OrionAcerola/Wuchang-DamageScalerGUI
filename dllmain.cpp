// main.dll
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <imgui.h>

#include <fstream>
#include <string>
#include <string_view>

// extras for clamping/formatting
#include <algorithm>  // std::max
#include <cmath>      // std::trunc
#include <iomanip>    // std::setprecision
#include <locale>     // std::locale::classic

class WuchangDamageScaler : public RC::CppUserModBase {
public:
    static constexpr const char* kCfgPath = "ue4ss/Mods/WuchangDamageScaler/dmg_mult.cfg";
    static constexpr float       kMin = 0.00f;
    static constexpr float       kAttackMin = 0.50f; // below this, anims can stall

    struct Settings {
        // Enemies
        float enemy_phys_mult = 1.00f;
        float enemy_elem_mult = 1.00f;
        float enemy_status_mult = 1.00f;

        // Player
        float player_phys_mult = 1.00f;
        float player_elem_mult = 1.00f;
        float player_status_mult = 1.00f;
        float player_health_mult = 1.00f;
        float player_stamina_mult = 1.00f;
		float player_move_spd = 1.00f;
		float player_attack_spd = 1.00f;
    };

    Settings s{};
    bool tab_registered = false;

    // --- helpers -------------------------------------------------------------
    static inline float trunc2(float x) {
        return std::trunc(x * 100.0f) / 100.0f;
    }

    static void clamp(Settings& v) {
        v.enemy_phys_mult = trunc2(std::max(v.enemy_phys_mult, kMin));
        v.enemy_elem_mult = trunc2(std::max(v.enemy_elem_mult, kMin));
        v.enemy_status_mult = trunc2(std::max(v.enemy_status_mult, kMin));
        v.player_phys_mult = trunc2(std::max(v.player_phys_mult, kMin));
        v.player_elem_mult = trunc2(std::max(v.player_elem_mult, kMin));
        v.player_status_mult = trunc2(std::max(v.player_status_mult, kMin));
        v.player_health_mult = trunc2(std::max(v.player_health_mult, kMin));
        v.player_stamina_mult = trunc2(std::max(v.player_stamina_mult, kMin));
        v.player_move_spd = trunc2(std::max(v.player_move_spd, kMin));
		v.player_attack_spd = trunc2(std::max(v.player_attack_spd, kAttackMin));
    }

    static bool parse_kv_line(const std::string& line, std::string& k, std::string& v) {
        auto pos = line.find('=');
        if (pos == std::string::npos) return false;
        k = line.substr(0, pos);
        v = line.substr(pos + 1);
        auto trim = [](std::string& x) {
            size_t a = x.find_first_not_of(" \t\r\n");
            size_t b = x.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) { x.clear(); return; }
            x = x.substr(a, b - a + 1);
            };
        trim(k); trim(v);
        return !k.empty() && !v.empty();
    }

    static bool to_float(std::string_view sv, float& out) {
        try {
            size_t idx = 0;
            out = std::stof(std::string(sv), &idx);
            return idx > 0;
        }
        catch (...) { return false; }
    }

    static void load_cfg(Settings& out) {
        Settings tmp = out;
        if (std::ifstream f(kCfgPath); f) {
            std::string line;
            while (std::getline(f, line)) {
                if (line.empty() || line[0] == '#' || line[0] == ';') continue;

                std::string k, v;
                if (!parse_kv_line(line, k, v)) continue;

                float num{};
                if (!to_float(v, num)) continue;

                if      (k == "enemy_phys_mult")     tmp.enemy_phys_mult = num;
                else if (k == "enemy_elem_mult")     tmp.enemy_elem_mult = num;
                else if (k == "enemy_status_mult")   tmp.enemy_status_mult = num;
                else if (k == "player_phys_mult")    tmp.player_phys_mult = num;
                else if (k == "player_elem_mult")    tmp.player_elem_mult = num;
                else if (k == "player_status_mult")  tmp.player_status_mult = num;
                else if (k == "player_health_mult")  tmp.player_health_mult = num;
                else if (k == "player_stamina_mult") tmp.player_stamina_mult = num;
                else if (k == "player_move_spd")     tmp.player_move_spd = num;
                else if (k == "player_attack_spd")   tmp.player_attack_spd = num;
            }
            clamp(tmp);
            out = tmp;
        }
        else {
            clamp(out);
        }
    }

    static void save_cfg(const Settings& v) {
        if (std::ofstream out(kCfgPath, std::ios::trunc); out) {
            out.imbue(std::locale::classic());
            out.setf(std::ios::fixed, std::ios::floatfield);
            out << std::setprecision(2);

            out << "# WuchangDamageScaler config (key=value)\n";
            out << "enemy_phys_mult=" << v.enemy_phys_mult << "\n";
            out << "enemy_elem_mult=" << v.enemy_elem_mult << "\n";
            out << "enemy_status_mult=" << v.enemy_status_mult << "\n";
            out << "player_phys_mult=" << v.player_phys_mult << "\n";
            out << "player_elem_mult=" << v.player_elem_mult << "\n";
            out << "player_status_mult=" << v.player_status_mult << "\n";
            out << "player_health_mult=" << v.player_health_mult << "\n";
            out << "player_stamina_mult=" << v.player_stamina_mult << "\n";
            out << "player_move_spd=" << v.player_move_spd << "\n";
            out << "player_attack_spd=" << v.player_attack_spd << "\n";
        }
    }

    // --- UI ------------------------------------------------------------------
    void on_ui_init() override {
        UE4SS_ENABLE_IMGUI();   // Ctrl+O toggles the overlay
        load_cfg(s);            // reflect current cfg

        if (tab_registered) return;

        register_tab(STR("Damage Scaler"), [](RC::CppUserModBase* base) {
            auto* self = static_cast<WuchangDamageScaler*>(base);
            ImGui::PushID("WuchangDS");

            ImGui::TextUnformatted("Damage Multipliers (0.00 = none, 0.50 = half, 1.00 = normal, 2.00 = double, etc)");
            ImGui::Separator();

            // ------------------ Enemies ------------------
            ImGui::PushID("Enemies");
            ImGui::TextUnformatted("Enemies");

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Physical Damage##WuchangDS", &self->s.enemy_phys_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.enemy_phys_mult = trunc2(std::max(self->s.enemy_phys_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Elemental Damage##WuchangDS", &self->s.enemy_elem_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.enemy_elem_mult = trunc2(std::max(self->s.enemy_elem_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Status Damage##WuchangDS", &self->s.enemy_status_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.enemy_status_mult = trunc2(std::max(self->s.enemy_status_mult, kMin));
            }
            ImGui::PopID();

            ImGui::Separator();

            // ------------------ Player -------------------
            ImGui::PushID("Player");
            ImGui::TextUnformatted("Player");

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Physical Damage##WuchangDS", &self->s.player_phys_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.player_phys_mult = trunc2(std::max(self->s.player_phys_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Elemental Damage##WuchangDS", &self->s.player_elem_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.player_elem_mult = trunc2(std::max(self->s.player_elem_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Status Damage##WuchangDS", &self->s.player_status_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.player_status_mult = trunc2(std::max(self->s.player_status_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Max Health##WuchangDS", &self->s.player_health_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.player_health_mult = trunc2(std::max(self->s.player_health_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Max Stamina##WuchangDS", &self->s.player_stamina_mult, 0.0f, 0.0f, "%.2f")) {
                self->s.player_stamina_mult = trunc2(std::max(self->s.player_stamina_mult, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Move Speed##WuchangDS", &self->s.player_move_spd, 0.0f, 0.0f, "%.2f")) {
                self->s.player_move_spd = trunc2(std::max(self->s.player_move_spd, kMin));
            }

            ImGui::SetNextItemWidth(140.0f);
            if (ImGui::InputFloat("Attack Speed##WuchangDS", &self->s.player_attack_spd, 0.0f, 0.0f, "%.2f")) {
                self->s.player_attack_spd = trunc2(std::max(self->s.player_attack_spd, kAttackMin));
            }
            ImGui::PopID();

            ImGui::Separator();

            if (ImGui::Button("Save##WuchangDS")) {
                clamp(self->s);
                save_cfg(self->s);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset##WuchangDS")) {
                self->s = Settings{};
                clamp(self->s);
                save_cfg(self->s);
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
