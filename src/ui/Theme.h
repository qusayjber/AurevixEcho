#pragma once
#include <imgui.h>

namespace aurevix {

struct Theme {
    static void apply_dark(float ui_scale = 1.0f);
    static void apply_light(float ui_scale = 1.0f);

    // Brand colors
    static ImVec4 accent();
    static ImVec4 text_primary();
    static ImVec4 text_secondary();
    static ImVec4 bg_primary();
    static ImVec4 bg_surface();
    static ImVec4 bg_elevated();
    static ImVec4 border();
    static ImU32 col(ImVec4 c, float alpha = 1.0f) {
        c.w *= alpha;
        return ImGui::GetColorU32(c);
    }
};

} // namespace aurevix