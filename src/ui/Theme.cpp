#include "ui/Theme.h"

namespace aurevix {

static ImVec4 C(int r, int g, int b, int a = 255) {
    return ImVec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

ImVec4 Theme::accent()         { return C(120, 160, 255); }
ImVec4 Theme::text_primary()   { return C(232, 234, 240); }
ImVec4 Theme::text_secondary() { return C(150, 156, 172); }
ImVec4 Theme::bg_primary()     { return C(16, 18, 24); }
ImVec4 Theme::bg_surface()     { return C(22, 25, 33); }
ImVec4 Theme::bg_elevated()    { return C(28, 32, 42); }
ImVec4 Theme::border()         { return C(40, 46, 60); }

void Theme::apply_dark(float ui_scale) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 12.0f;
    s.FrameRounding  = 10.0f;
    s.ChildRounding  = 12.0f;
    s.PopupRounding  = 10.0f;
    s.GrabRounding   = 8.0f;
    s.TabRounding    = 10.0f;
    s.ScrollbarRounding = 8.0f;
    s.WindowBorderSize = 0.0f;
    s.FrameBorderSize  = 0.0f;
    s.WindowPadding    = ImVec2(20, 20);
    s.FramePadding     = ImVec2(14, 8);
    s.ItemSpacing      = ImVec2(10, 10);
    s.ScrollbarSize    = 10.0f;
    s.GrabMinSize      = 10.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = text_primary();
    c[ImGuiCol_TextDisabled]         = text_secondary();
    c[ImGuiCol_WindowBg]             = bg_primary();
    c[ImGuiCol_ChildBg]              = bg_surface();
    c[ImGuiCol_PopupBg]              = bg_elevated();
    c[ImGuiCol_Border]               = border();
    c[ImGuiCol_FrameBg]              = bg_elevated();
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.16f, 0.19f, 0.25f, 1.0f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.20f, 0.24f, 0.32f, 1.0f);
    c[ImGuiCol_TitleBg]              = bg_primary();
    c[ImGuiCol_TitleBgActive]        = bg_primary();
    c[ImGuiCol_MenuBarBg]            = bg_primary();
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0,0,0,0);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.22f, 0.25f, 0.32f, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.32f, 0.40f, 1.0f);
    c[ImGuiCol_CheckMark]            = accent();
    c[ImGuiCol_SliderGrab]           = accent();
    c[ImGuiCol_SliderGrabActive]     = accent();
    c[ImGuiCol_Button]               = ImVec4(0.14f, 0.17f, 0.23f, 1.0f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.19f, 0.23f, 0.31f, 1.0f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.24f, 0.29f, 0.39f, 1.0f);
    c[ImGuiCol_Header]               = ImVec4(0.14f, 0.17f, 0.23f, 1.0f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.19f, 0.23f, 0.31f, 1.0f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.24f, 0.29f, 0.39f, 1.0f);
    c[ImGuiCol_Separator]            = border();
    c[ImGuiCol_Tab]                  = bg_surface();
    c[ImGuiCol_TabHovered]           = bg_elevated();
    c[ImGuiCol_TabActive]            = bg_elevated();

    ImGui::GetIO().FontGlobalScale = ui_scale;
}

void Theme::apply_light(float ui_scale) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 12.0f;
    s.FrameRounding  = 10.0f;
    s.ChildRounding  = 12.0f;
    s.PopupRounding  = 10.0f;
    s.GrabRounding   = 8.0f;
    s.TabRounding    = 10.0f;
    s.WindowPadding  = ImVec2(20, 20);
    s.FramePadding   = ImVec2(14, 8);
    s.ItemSpacing    = ImVec2(10, 10);
    s.ScrollbarSize  = 10.0f;

    auto bg  = C(246, 247, 250);
    auto sfc = C(255, 255, 255);
    auto brd = C(220, 223, 230);
    auto txt = C(28, 32, 42);
    auto sub = C(110, 116, 130);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = txt;
    c[ImGuiCol_TextDisabled]         = sub;
    c[ImGuiCol_WindowBg]             = bg;
    c[ImGuiCol_ChildBg]              = sfc;
    c[ImGuiCol_PopupBg]              = sfc;
    c[ImGuiCol_Border]               = brd;
    c[ImGuiCol_FrameBg]              = C(240, 242, 246);
    c[ImGuiCol_FrameBgHovered]       = C(232, 235, 242);
    c[ImGuiCol_FrameBgActive]        = C(224, 228, 238);
    c[ImGuiCol_Button]               = C(236, 239, 246);
    c[ImGuiCol_ButtonHovered]        = C(224, 229, 240);
    c[ImGuiCol_ButtonActive]         = C(212, 220, 236);
    c[ImGuiCol_Header]               = C(236, 239, 246);
    c[ImGuiCol_HeaderHovered]        = C(224, 229, 240);
    c[ImGuiCol_HeaderActive]         = C(212, 220, 236);
    c[ImGuiCol_CheckMark]            = accent();
    c[ImGuiCol_SliderGrab]           = accent();
    c[ImGuiCol_SliderGrabActive]     = accent();
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0,0,0,0);
    c[ImGuiCol_ScrollbarGrab]        = C(200, 205, 216);
    c[ImGuiCol_ScrollbarGrabHovered] = C(180, 186, 200);
    c[ImGuiCol_Separator]            = brd;
    c[ImGuiCol_Tab]                  = sfc;
    c[ImGuiCol_TabHovered]           = C(240, 242, 246);
    c[ImGuiCol_TabActive]            = C(232, 235, 242);

    ImGui::GetIO().FontGlobalScale = ui_scale;
}

} // namespace aurevix