#include "ui/UI.h"
#include "ui/Theme.h"
#include "analytics/SessionAnalyzer.h"
#include "storage/Database.h"
#include "storage/Repositories.h"
#include "utils/Export.h"
#include "core/TimeManager.h"
#include "core/Logger.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <misc/cpp/imgui_stdlib.h>

#include <d3d11.h>
#include <tchar.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <cctype>      // NEW: std::tolower
#include <cmath>
#include <functional>  // NEW: std::hash, std::function
#include <map>
#include <sstream>
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace aurevix {

// --- Local helper: builds an ImU32 from an ImVec4 with an explicit alpha multiplier ---
static ImU32 colA(ImVec4 c, float alpha = 1.0f) {
    c.w *= alpha;
    return ImGui::GetColorU32(c);
}

UI::UI() {
    selected_day_ = start_of_day(now());
}
UI::~UI() { destroy(); }

// ---------- D3D ----------
bool UI::create_device_d3d(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels, 2,
        D3D11_SDK_VERSION, &sd, &swap_, &device_, &got, &ctx_);
    if (FAILED(hr)) return false;
    create_rtv();
    return true;
}

void UI::create_rtv() {
    ID3D11Texture2D* back = nullptr;
    swap_->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { device_->CreateRenderTargetView(back, nullptr, &rtv_); back->Release(); }
}

void UI::destroy_rtv() { if (rtv_) { rtv_->Release(); rtv_ = nullptr; } }

void UI::destroy_device_d3d() {
    destroy_rtv();
    if (swap_) { swap_->Release(); swap_ = nullptr; }
    if (ctx_)  { ctx_->Release();  ctx_  = nullptr; }
    if (device_){ device_->Release(); device_ = nullptr; }
}

bool UI::create(HWND hwnd) {
    hwnd_ = hwnd;
    if (!create_device_d3d(hwnd)) return false;
    dx_ok_ = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    Theme::apply_dark(1.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device_, ctx_);
    return true;
}

void UI::destroy() {
    if (dx_ok_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        destroy_device_d3d();
        dx_ok_ = false;
    }
}

void UI::on_resize(unsigned w, unsigned h) {
    if (!dx_ok_ || !swap_ || !ctx_) return;
    if (w == 0 || h == 0) return;
    destroy_rtv();
    swap_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    create_rtv();
}

void UI::begin_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("root", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground);
}

void UI::end_frame() {
    ImGui::End();
    ImGui::Render();
    const float clear[4] = { 0.06f, 0.07f, 0.09f, 1.0f };
    ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
    ctx_->ClearRenderTargetView(rtv_, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swap_->Present(1, 0);
}

// ---------- Helpers ----------
void UI::section_header(const char* title, const char* subtitle) {
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
    ImGui::SetWindowFontScale(1.35f);
    ImGui::TextUnformatted(title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    if (subtitle && *subtitle) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextUnformatted(subtitle);
        ImGui::PopStyleColor();
    }
    ImGui::Dummy(ImVec2(0, 6));
}

void UI::card(const char* title, const char* value, const char* subtitle) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild(title, ImVec2(0, 108), true,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
    ImGui::SetWindowFontScale(1.9f);
    ImGui::TextUnformatted(value);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    if (subtitle && *subtitle) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextUnformatted(subtitle);
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void UI::metric_bar(const char* label, float v, const char* hint) {
    v = std::clamp(v, 0.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    if (hint && *hint) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextUnformatted(hint);
        ImGui::PopStyleColor();
    }
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::accent());
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::bg_elevated());
    ImGui::ProgressBar(v, ImVec2(-1, 10), "");
    ImGui::PopStyleColor(2);
    ImGui::Dummy(ImVec2(0, 4));
}

// ---------- Sidebar ----------
void UI::draw_sidebar() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##sidebar", ImVec2(sidebar_collapsed_ ? 68 : 220, 0), true,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();

    // Logo
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        auto* dl = ImGui::GetWindowDrawList();
        float cx = p.x + 22, cy = p.y + 22;
        for (int r = 22; r >= 6; r -= 6) {
            // FIX (C4244): split into explicit float steps.
            const float ring_t = static_cast<float>(22 - r) / 16.0f;
            const float alpha  = 0.15f + 0.15f * ring_t;
            dl->AddCircle(ImVec2(cx, cy), static_cast<float>(r),
                          colA(Theme::accent(), alpha),
                          48, 1.5f);
        }
        dl->AddCircleFilled(ImVec2(cx, cy), 4.0f, ImGui::GetColorU32(Theme::accent()));
        ImGui::Dummy(ImVec2(44, 44));
        if (!sidebar_collapsed_) {
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
            ImGui::TextUnformatted("AUREVIX ECHO");
        }
    }
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 6));

    auto nav = [&](const char* label, Page p, const char* icon) {
        bool active = (page_ == p) && !onboarding_;
        if (active) ImGui::PushStyleColor(ImGuiCol_Header, colA(Theme::accent(), 0.20f));
        std::string txt = std::string(icon) + "  " + (sidebar_collapsed_ ? "" : label);
        if (ImGui::Selectable(txt.c_str(), active,
                              ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 34))) {
            page_ = p;
        }
        if (active) ImGui::PopStyleColor();
    };

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    if (!sidebar_collapsed_) ImGui::TextUnformatted("ECHO");
    ImGui::PopStyleColor();

    nav("Overview", Page::Overview, "\xE2\x97\x8F");
    nav("Timeline", Page::Timeline, "\xE2\x94\x80");
    nav("Workflow", Page::Workflow, "\xE2\x97\x87");
    nav("Patterns", Page::Patterns, "\xE2\x96\xA3");
    nav("Replay",   Page::Replay,   "\xE2\x96\xB6");
    nav("Insights", Page::Insights, "\xE2\x9C\xA6");

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    if (!sidebar_collapsed_) ImGui::TextUnformatted("SYSTEM");
    ImGui::PopStyleColor();

    nav("System Pulse", Page::System,  "\xE2\x9A\x99");
    nav("Privacy",      Page::Privacy, "\xE2\x9B\xA8");
    nav("Settings",     Page::Settings,"\xE2\x9A\x99");

    // collapse toggle at bottom
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 44);
    if (ImGui::Button(sidebar_collapsed_ ? ">" : "<", ImVec2(-1, 28))) {
        sidebar_collapsed_ = !sidebar_collapsed_;
    }

    ImGui::EndChild();
}

// ---------- Onboarding ----------
void UI::draw_onboarding(Configuration& cfg, std::function<void()> on_config_save) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(vp->WorkSize.x * 0.5f, vp->WorkSize.y * 0.5f);

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(720, 460), ImGuiCond_Always);
    ImGui::Begin("##onboarding", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##onb_inner", ImVec2(0, -50), true);

    switch (onb_step_) {
        case OnboardingStep::Welcome: {
            ImGui::Dummy(ImVec2(0, 40));
            ImGui::SetWindowFontScale(2.4f);
            ImGui::TextUnformatted("AUREVIX ECHO");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::TextWrapped("Your computer remembers how you use it.");
            ImGui::Dummy(ImVec2(0, 12));
            ImGui::TextWrapped("Echo turns everyday computer activity into patterns you can actually see.");
            ImGui::PopStyleColor();
            break;
        }
        case OnboardingStep::Privacy: {
            ImGui::Dummy(ImVec2(0, 40));
            ImGui::SetWindowFontScale(1.9f);
            ImGui::TextUnformatted("PRIVATE BY DESIGN");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::TextWrapped("Everything stays on this computer.");
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::TextWrapped("No cloud.  No account.  No API key.  No content collection.");
            ImGui::PopStyleColor();
            break;
        }
        case OnboardingStep::WhatIsCollected: {
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::SetWindowFontScale(1.6f);
            ImGui::TextUnformatted("What Echo collects");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Dummy(ImVec2(0, 12));
            ImGui::Columns(2, nullptr, false);
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
            ImGui::TextUnformatted("Collected");
            ImGui::PopStyleColor();
            ImGui::BulletText("Foreground application name");
            ImGui::BulletText("Session start / end / duration");
            ImGui::BulletText("Application switch events");
            ImGui::BulletText("CPU / RAM / disk / network totals");
            ImGui::BulletText("Idle periods");

            ImGui::NextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::TextUnformatted("Never collected");
            ImGui::PopStyleColor();
            ImGui::BulletText("Keystrokes or typed text");
            ImGui::BulletText("Passwords or clipboard");
            ImGui::BulletText("Window titles or file names");
            ImGui::BulletText("Screenshots, mic, camera");
            ImGui::BulletText("Browser content");
            ImGui::Columns(1);
            break;
        }
        case OnboardingStep::Configure: {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::SetWindowFontScale(1.5f);
            ImGui::TextUnformatted("Set your preferences");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Checkbox("Enable monitoring", &cfg.monitoring_enabled);
            ImGui::Checkbox("Launch on startup", &cfg.startup_enabled);
            ImGui::SliderInt("Retention (days)", &cfg.retention_days, 7, 365);
            int theme = static_cast<int>(cfg.theme);
            const char* items[] = { "Dark", "Light", "System" };
            if (ImGui::Combo("Theme", &theme, items, 3))
                cfg.theme = static_cast<ThemeMode>(theme);
            break;
        }
        default: break;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    bool last = (onb_step_ == OnboardingStep::Configure);
    if (last) {
        if (ImGui::Button("START ECHO", ImVec2(160, 36))) {
            cfg.first_run_completed = true;
            onboarding_ = false;
            on_config_save();
        }
    } else {
        if (ImGui::Button("Continue", ImVec2(160, 36))) {
            onb_step_ = static_cast<OnboardingStep>(int(onb_step_) + 1);
        }
        ImGui::SameLine();
        if (onb_step_ != OnboardingStep::Welcome) {
            if (ImGui::Button("Back", ImVec2(100, 36))) {
                onb_step_ = static_cast<OnboardingStep>(int(onb_step_) - 1);
            }
        }
    }

    ImGui::End();
}

// ---------- Overview ----------
void UI::draw_overview(ActivityMonitor& monitor) {
    auto snap = monitor.snapshot();
    Timestamp today = start_of_day(now());

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
    ImGui::SetWindowFontScale(1.6f);
    ImGui::TextUnformatted(greeting_for_now().c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::TextUnformatted("Here's what Echo discovered today.");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 12));

    auto active_seconds = SessionAnalyzer::total_active_seconds(today, today + 86400);
    auto focus = snap.focus_sessions_today;
    auto switches = SessionAnalyzer::switch_count(today, today + 86400);
    auto uniq = SessionAnalyzer::unique_app_count(today, today + 86400);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 14));
    ImGui::BeginTable("##cards", 4, ImGuiTableFlags_SizingStretchSame);
    ImGui::TableNextColumn(); card("ACTIVE TIME", format_human_duration(active_seconds).c_str());
    ImGui::TableNextColumn(); card("FOCUS SESSIONS", std::to_string(focus).c_str());
    ImGui::TableNextColumn(); card("APP SWITCHES", std::to_string(switches).c_str());
    ImGui::TableNextColumn(); card("ACTIVE APPLICATIONS", std::to_string(uniq).c_str());
    ImGui::EndTable();
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, 20));
    section_header("Current Activity");
    if (!snap.current_app.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
        ImGui::BeginChild("##cur", ImVec2(0, 84), true);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::SetWindowFontScale(1.4f);
        ImGui::TextUnformatted(snap.current_app.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::Text("Started %s ago", format_human_duration(snap.current_seconds).c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Currently idle. Echo is standing by.");
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0, 20));
    section_header("Today's Flow");
    draw_flow_ribbon(snap.recent_flow);

    ImGui::Dummy(ImVec2(0, 20));
    section_header("Today's Top Applications");
    auto apps = SessionAnalyzer::top_apps(today, today + 86400, 6);
    if (apps.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Echo is learning. Your first discoveries will appear here as you use your computer normally.");
        ImGui::PopStyleColor();
    } else {
        std::int64_t max_sec = apps.empty() ? 1 : apps.front().seconds;
        for (auto& a : apps) {
            ImGui::Text("%s", a.app_name.c_str());
            ImGui::SameLine();
            float frac = float(double(a.seconds) / double(std::max<std::int64_t>(1, max_sec)));
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::accent());
            ImGui::ProgressBar(frac, ImVec2(-1, 8), "");
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::Text("   %s  .  %lld sessions", format_human_duration(a.seconds).c_str(),
                        (long long)a.sessions);
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));
        }
    }
}

void UI::draw_flow_ribbon(const std::vector<std::string>& flow) {
    if (flow.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("No activity yet today.");
        ImGui::PopStyleColor();
        return;
    }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##flow", ImVec2(0, 80), true);
    for (size_t i = 0; i < flow.size(); ++i) {
        if (i) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::TextUnformatted("  \xE2\x86\x92  ");
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        bool active = (i + 1 == flow.size());
        if (active) ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
        else        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::TextUnformatted(flow[i].c_str());
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ---------- Timeline ----------
void UI::draw_timeline() {
    section_header("Timeline", "A visual record of how today's attention moved between applications.");

    ImGui::BeginChild("##tl_ctl", ImVec2(0, 44), false);
    if (ImGui::Button("< Prev day")) selected_day_ -= 86400;
    ImGui::SameLine();
    if (ImGui::Button("Today")) selected_day_ = start_of_day(now());
    ImGui::SameLine();
    if (ImGui::Button("Next day >")) selected_day_ += 86400;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::Text("   %s", format_day(selected_day_).c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();

    auto sessions = SessionRepository::for_day(selected_day_);
    draw_timeline_ribbon(selected_day_, selected_day_ + 86400, sessions);

    ImGui::Dummy(ImVec2(0, 20));
    section_header("Sessions", "Ordered chronologically.");

    if (sessions.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Echo is learning. There isn't enough activity yet for this day.");
        ImGui::PopStyleColor();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##slist", ImVec2(0, 0), true);
    for (auto& s : sessions) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::Text("%s", format_clock(s.started_at).c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(90);
        ImGui::Text("%s", s.app_name.c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::Text("  %s", format_human_duration(s.duration_seconds).c_str());
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void UI::draw_timeline_ribbon(Timestamp dayStart, Timestamp dayEnd,
                              const std::vector<SessionRow>& sessions) {
    (void)dayEnd;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##ribbon", ImVec2(0, 120), true);
    ImGui::PopStyleColor();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size   = ImGui::GetContentRegionAvail();
    float  yTop   = origin.y + size.y * 0.5f - 12;
    float  yBot   = yTop + 24;

    for (int h = 0; h <= 24; h += 2) {
        float x = origin.x + size.x * (float(h) / 24.0f);
        dl->AddLine(ImVec2(x, yTop - 6), ImVec2(x, yBot + 22),
                    ImGui::GetColorU32(Theme::border()));
        char buf[8]; std::snprintf(buf, sizeof(buf), "%02d", h);
        dl->AddText(ImVec2(x + 2, yBot + 4),
                    ImGui::GetColorU32(Theme::text_secondary()), buf);
    }

    for (auto& s : sessions) {
        float t0 = float(s.started_at - dayStart) / 86400.0f;
        float t1 = float(s.ended_at   - dayStart) / 86400.0f;
        if (t1 <= 0 || t0 >= 1) continue;
        t0 = std::clamp(t0, 0.0f, 1.0f);
        t1 = std::clamp(t1, 0.0f, 1.0f);
        float x0 = origin.x + size.x * t0;
        float x1 = origin.x + size.x * t1;
        if (x1 - x0 < 1.5f) x1 = x0 + 1.5f;

        size_t h = std::hash<std::string>{}(s.app_name);
        float hue = float(h % 360) / 360.0f;
        ImColor base = ImColor::HSV(hue, 0.55f, 0.85f);

        dl->AddRectFilled(ImVec2(x0, yTop), ImVec2(x1, yBot), (ImU32)base, 4.0f);

        if (ImGui::IsMouseHoveringRect(ImVec2(x0, yTop), ImVec2(x1, yBot))) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", s.app_name.c_str());
            ImGui::Text("%s -> %s  (%s)",
                        format_clock(s.started_at).c_str(),
                        format_clock(s.ended_at).c_str(),
                        format_human_duration(s.duration_seconds).c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::Dummy(size);
    ImGui::EndChild();
}

// ---------- Workflow ----------
void UI::draw_workflow() {
    section_header("Workflow Graph",
                   "Nodes are applications. Edges are transitions. Thicker edges mean more frequent movement.");

    Timestamp now_t = now();
    Timestamp window = 7 * 86400;

    if (wgraph_window_ == 0 || now_t - wgraph_window_ > 60) {
        wgraph_.build(now_t - window, now_t, 1);
        wgraph_window_ = now_t;
    }

    ImGui::BeginChild("##wg_ctl", ImVec2(0, 40), false);
    ImGui::Text("Window: last 7 days   .   Nodes: %zu   .   Edges: %zu",
                wgraph_.nodes().size(), wgraph_.edges().size());
    if (ImGui::Button("Rebuild")) {
        wgraph_.build(now_t - window, now_t, 1);
    }
    ImGui::SameLine();
    if (ImGui::Button("Relayout")) wgraph_.layout_circle();
    ImGui::EndChild();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##wg_canvas", ImVec2(0, 480), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImVec2 center = ImVec2(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);

    wgraph_.step_physics(0.05f);

    auto& nodes = wgraph_.nodes();
    auto& edges = wgraph_.edges();
    std::int64_t max_count = 1;
    for (auto& e : edges) if (e.count > max_count) max_count = e.count;

    for (auto& e : edges) {
        if (e.from < 0 || e.to < 0 ||
            e.from >= (int)nodes.size() || e.to >= (int)nodes.size()) continue;
        auto& a = nodes[e.from];
        auto& b = nodes[e.to];
        float w = 1.0f + 5.0f * float(double(e.count) / double(max_count));
        ImVec2 p1(center.x + a.x, center.y + a.y);
        ImVec2 p2(center.x + b.x, center.y + b.y);
        dl->AddLine(p1, p2, colA(Theme::accent(), 0.15f + 0.35f * w / 6.0f), w);
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        auto& n = nodes[i];
        ImVec2 p(center.x + n.x, center.y + n.y);
        ImColor base = ImColor::HSV(float(std::hash<std::string>{}(n.name) % 360) / 360.0f,
                                    0.5f, 0.85f);
        dl->AddCircleFilled(p, n.radius, (ImU32)base);
        dl->AddCircle(p, n.radius, ImGui::GetColorU32(Theme::bg_primary()), 0, 2.0f);

        ImVec2 ts = ImGui::CalcTextSize(n.name.c_str());
        dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y + n.radius + 4),
                    ImGui::GetColorU32(Theme::text_primary()), n.name.c_str());

        if (ImGui::IsMouseHoveringRect(ImVec2(p.x - n.radius, p.y - n.radius),
                                       ImVec2(p.x + n.radius, p.y + n.radius))) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", n.name.c_str());
            ImGui::Separator();
            ImGui::Text("Total: %s", format_human_duration(n.seconds).c_str());
            ImGui::Text("Visits: %lld", (long long)n.visits);
            ImGui::EndTooltip();
        }
    }

    ImGui::Dummy(size);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 12));
    section_header("Top transitions", "Ranked by frequency of movement.");
    auto counts = SessionAnalyzer::transition_counts(now_t - window, now_t, 12);
    if (counts.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Echo is learning. Not enough transitions yet.");
        ImGui::PopStyleColor();
    } else {
        for (auto& c : counts) {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
            ImGui::Text("%s", c.from_name.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::TextUnformatted("->");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%s", c.to_name.c_str());
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
            ImGui::Text("   x%lld", (long long)c.count);
            ImGui::PopStyleColor();
        }
    }
}

// ---------- Patterns ----------
void UI::draw_patterns() {
    section_header("Patterns",
                   "Recurring structures Echo has observed in your activity. Derived only from stored data.");

    if (now() - patterns_built_ > 60) {
        patterns_ = PatternDetector::detect(now() - 7 * 86400, now());
        patterns_built_ = now();
    }

    if (patterns_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped(
            "Echo is learning.\n\n"
            "There isn't enough activity yet to discover meaningful patterns.\n"
            "Keep using your computer normally. Your first discoveries will appear here\n"
            "when enough real data is available.");
        ImGui::PopStyleColor();
        return;
    }

    for (auto& p : patterns_) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
        ImGui::BeginChild(("##pat_" + p.kind + p.title).c_str(), ImVec2(0, 118), true);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
        ImGui::TextUnformatted(p.kind.c_str());
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::SetWindowFontScale(1.25f);
        ImGui::TextUnformatted(p.title.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("%s", p.detail.c_str());
        ImGui::Text("Observed: %lld  .  Sample: %lld  .  Confidence: %.0f%%",
                    (long long)p.occurrences, (long long)p.sample_size,
                    p.confidence * 100.0f);
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));
    }
}

// ---------- Replay ----------
void UI::draw_replay() {
    section_header("Replay", "Watch a day of your digital life replayed.");
    if (ImGui::Button("< Prev day")) selected_day_ -= 86400;
    ImGui::SameLine();
    if (ImGui::Button("Today")) selected_day_ = start_of_day(now());
    ImGui::SameLine();
    if (ImGui::Button("Next day >")) selected_day_ += 86400;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::Text("   %s", format_day(selected_day_).c_str());
    ImGui::PopStyleColor();

    auto sessions = SessionRepository::for_day(selected_day_);

    ImGui::Dummy(ImVec2(0, 16));
    if (ImGui::Button(replay_playing_ ? "||  Pause" : ">  Play", ImVec2(120, 34)))
        replay_playing_ = !replay_playing_;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    ImGui::SliderFloat("Speed", &replay_speed_, 1.0f, 10.0f, "%.1fx");
    ImGui::SameLine();
    if (ImGui::Button("Reset")) { replay_time_ = 0.0f; replay_playing_ = false; }

    if (replay_playing_) {
        replay_time_ += ImGui::GetIO().DeltaTime * replay_speed_ / 30.0f;
        if (replay_time_ >= 1.0f) { replay_time_ = 1.0f; replay_playing_ = false; }
    }
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##scrub", &replay_time_, 0.0f, 1.0f, "");

    ImGui::Dummy(ImVec2(0, 12));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##rpl", ImVec2(0, 260), true);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    const float yTop = origin.y + 40;
    const float yBot = origin.y + 200;
    const float cursorX = origin.x + size.x * replay_time_;

    for (auto& s : sessions) {
        float t0 = float(s.started_at - selected_day_) / 86400.0f;
        float t1 = float(s.ended_at   - selected_day_) / 86400.0f;
        if (t1 <= 0 || t0 >= 1 || t0 > replay_time_) continue;
        t1 = std::min(t1, replay_time_);
        float x0 = origin.x + size.x * std::clamp(t0, 0.0f, 1.0f);
        float x1 = origin.x + size.x * std::clamp(t1, 0.0f, 1.0f);
        if (x1 - x0 < 2) x1 = x0 + 2;
        ImColor base = ImColor::HSV(float(std::hash<std::string>{}(s.app_name) % 360) / 360.0f,
                                    0.5f, 0.85f);
        dl->AddRectFilled(ImVec2(x0, yTop), ImVec2(x1, yBot), (ImU32)base, 4.0f);
    }

    dl->AddLine(ImVec2(cursorX, yTop - 20), ImVec2(cursorX, yBot + 20),
                ImGui::GetColorU32(Theme::accent()), 2.0f);
    Timestamp cursor_ts = selected_day_ + Timestamp(replay_time_ * 86400.0f);
    dl->AddText(ImVec2(cursorX + 4, yTop - 34),
                ImGui::GetColorU32(Theme::text_primary()), format_clock(cursor_ts).c_str());

    ImGui::SetCursorScreenPos(ImVec2(origin.x, yBot + 34));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::TextUnformatted("Active at cursor:");
    ImGui::PopStyleColor();
    for (auto& s : sessions) {
        if (s.started_at <= cursor_ts && s.ended_at >= cursor_ts) {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
            ImGui::BulletText("%s   .   %s", s.app_name.c_str(),
                              format_human_duration(s.duration_seconds).c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::Dummy(size);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ---------- Insights ----------
void UI::draw_insights() {
    section_header("Insights", "Concrete observations backed by your stored data.");

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##q", ImVec2(0, 110), true);
    ImGui::TextUnformatted("Ask Echo");
    ImGui::SetNextItemWidth(-160);
    ImGui::InputText("##qtext", &query_text_);
    ImGui::SameLine();
    if (ImGui::Button("Ask", ImVec2(120, 28))) {
        query_result_ = evaluate_query(query_text_);
    }
    if (!query_result_.empty()) {
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("%s", query_result_.c_str());
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Try:  \"What did I use most today?\"  |  \"How many times did I switch from Chrome to IntelliJ?\"  |  \"What was my longest session?\"");
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 16));

    if (now() - insights_built_ > 60) {
        insights_ = PatternDetector::detect(now() - 86400, now());
        insights_built_ = now();
    }

    Timestamp today = start_of_day(now());

    auto top = SessionAnalyzer::top_apps(today, today + 86400, 1);
    if (!top.empty() && top.front().seconds > 60) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
        ImGui::BeginChild("##ins1", ImVec2(0, 120), true);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
        ImGui::TextUnformatted("TODAY");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextWrapped("Your most-used application today was %s.", top.front().app_name.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::Text("Total time: %s across %lld sessions.",
                    format_human_duration(top.front().seconds).c_str(),
                    (long long)top.front().sessions);
        ImGui::Text("Source:  sessions table, %s..%s  .  Sample: %lld",
                    format_clock(today).c_str(),
                    format_clock(today + 86400).c_str(),
                    (long long)top.front().sessions);
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));
    }

    auto longest = SessionAnalyzer::longest_session(today, today + 86400);
    if (longest && longest->duration_seconds >= 300) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
        ImGui::BeginChild("##ins2", ImVec2(0, 120), true);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
        ImGui::TextUnformatted("DISCOVERY");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextWrapped("Your longest uninterrupted session today was %s in %s.",
                           format_human_duration(longest->duration_seconds).c_str(),
                           longest->app_name.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::Text("Started at %s.", format_clock(longest->started_at).c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));
    }

    for (auto& p : insights_) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
        ImGui::BeginChild(("##pins_" + p.kind).c_str(), ImVec2(0, 118), true);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
        ImGui::TextUnformatted("DISCOVERY");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextWrapped("%s: %s", p.title.c_str(), p.detail.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::Text("Observed %lld times over the last 24h. Confidence %.0f%%.",
                    (long long)p.occurrences, p.confidence * 100.0f);
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 8));
    }

    if (top.empty() && !longest && insights_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Echo is learning. Insights will appear here as you use your computer.");
        ImGui::PopStyleColor();
    }
}

// ---------- System Pulse ----------
void UI::draw_system() {
    section_header("System Pulse", "The computer itself, correlated with your workflow.");

    Timestamp now_t = now();
    Timestamp from = now_t - 3600;
    if (now_t - last_system_fetch_ > 3) {
        system_series_ = SystemMetricsRepository::between(from, now_t, 3600);
        last_system_fetch_ = now_t;
    }

    if (system_series_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("Waiting for the first system samples...");
        ImGui::PopStyleColor();
        return;
    }

    auto draw_spark = [&](const char* label, const char* unit,
                          std::function<float(const SystemMetricRow&)> getter,
                          float ymax) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::Text("%s (%s)", label, unit);
        ImGui::PopStyleColor();
        std::vector<float> vals;
        vals.reserve(system_series_.size());
        for (auto& r : system_series_) vals.push_back(std::clamp(getter(r), 0.0f, ymax));
        std::string id = std::string("##spark_") + label;
        ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::accent());
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::bg_elevated());
        ImGui::PlotLines(id.c_str(), vals.data(), (int)vals.size(), 0, nullptr,
                         0.0f, ymax, ImVec2(-1, 70));
        ImGui::PopStyleColor(2);
        ImGui::Dummy(ImVec2(0, 8));
    };

    draw_spark("CPU", "%", [](const SystemMetricRow& r){ return float(r.cpu_pct); }, 100.0f);
    draw_spark("RAM", "%", [](const SystemMetricRow& r){ return float(r.ram_pct); }, 100.0f);
    draw_spark("Disk", "%", [](const SystemMetricRow& r){ return float(r.disk_pct); }, 100.0f);

    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_primary());
        ImGui::TextUnformatted("Network (KiB/s)");
        ImGui::PopStyleColor();
        float ymax = 1.0f;
        for (auto& r : system_series_) ymax = std::max(ymax, float(r.net_kbps));
        std::vector<float> vals;
        for (auto& r : system_series_) vals.push_back(float(r.net_kbps));
        ImGui::PushStyleColor(ImGuiCol_PlotLines, Theme::accent());
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::bg_elevated());
        ImGui::PlotLines("##net", vals.data(), (int)vals.size(), 0, nullptr,
                         0.0f, ymax, ImVec2(-1, 70));
        ImGui::PopStyleColor(2);
    }

    ImGui::Dummy(ImVec2(0, 16));
    section_header("Correlation", "Where system pressure overlapped with your workflow.");
    auto counts = SessionAnalyzer::top_apps(now_t - 3600, now_t, 4);
    if (counts.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
        ImGui::TextWrapped("No workflow sessions in the last hour.");
        ImGui::PopStyleColor();
    } else {
        for (auto& a : counts) {
            ImGui::BulletText("%s  .  %s", a.app_name.c_str(),
                              format_human_duration(a.seconds).c_str());
        }
    }
}

// ---------- Privacy ----------
void UI::draw_privacy(const Configuration& cfg,
                      Configuration& cfg_mutable,
                      std::function<void()> on_config_save,
                      std::function<void()> on_pause_monitoring,
                      std::function<void()> on_resume_monitoring,
                      std::function<void()> on_wipe_data) {
    section_header("Privacy Center", "Echo is local-only by design.");

    auto size_bytes = Database::instance().file_size_bytes();
    double mb = double(size_bytes) / (1024.0 * 1024.0);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##priv1", ImVec2(0, 160), true);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
    ImGui::TextUnformatted("LOCAL ONLY");
    ImGui::PopStyleColor();
    ImGui::Text("Data stored on this device:  %.2f MB", mb);
    ImGui::Text("Network transmission by Echo:  0 bytes");
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::TextWrapped("Echo performs no outbound network requests. It does not open any network ports.");
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 16));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_surface());
    ImGui::BeginChild("##priv2", ImVec2(0, 300), true);
    ImGui::TextUnformatted("Controls");
    ImGui::Dummy(ImVec2(0, 8));

    bool mon = cfg.monitoring_enabled;
    if (ImGui::Checkbox("Monitoring", &mon)) {
        cfg_mutable.monitoring_enabled = mon;
        if (mon) on_resume_monitoring(); else on_pause_monitoring();
        on_config_save();
    }
    bool start = cfg.startup_enabled;
    if (ImGui::Checkbox("Start with Windows", &start)) {
        cfg_mutable.startup_enabled = start;
        on_config_save();
    }
    int retention = cfg.retention_days;
    if (ImGui::SliderInt("Retention (days)", &retention, 7, 365)) {
        cfg_mutable.retention_days = retention;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) on_config_save();

    ImGui::Dummy(ImVec2(0, 12));
    if (ImGui::Button("Pause Monitoring", ImVec2(160, 32))) on_pause_monitoring();
    ImGui::SameLine();
    if (ImGui::Button("Resume Monitoring", ImVec2(160, 32))) on_resume_monitoring();
    ImGui::SameLine();
    if (ImGui::Button("Export JSON", ImVec2(140, 32))) {
        auto p = Exporter::export_json(start_of_day(now()) - 30 * 86400,
                                       now(),
                                       Exporter::default_export_dir());
        if (!p.empty()) ShellExecuteW(nullptr, L"open",
                                      Exporter::default_export_dir().c_str(),
                                      nullptr, nullptr, SW_SHOW);
    }
    ImGui::SameLine();
    if (ImGui::Button("Export CSV", ImVec2(120, 32))) {
        Exporter::export_csv(start_of_day(now()) - 30 * 86400,
                             now(), Exporter::default_export_dir());
    }

    ImGui::Dummy(ImVec2(0, 8));
    if (ImGui::Button("Open Data Folder", ImVec2(160, 32))) {
        PWSTR raw = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw))) {
            std::filesystem::path p = raw; p /= L"AurevixEcho";
            CoTaskMemFree(raw);
            ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOW);
        }
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
    if (ImGui::Button("Delete All Data", ImVec2(160, 32))) {
        ImGui::OpenPopup("ConfirmDelete");
    }
    ImGui::PopStyleColor();

    if (ImGui::BeginPopupModal("ConfirmDelete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("This permanently deletes every observation Echo has recorded. This cannot be undone.");
        ImGui::Dummy(ImVec2(0, 10));
        if (ImGui::Button("Delete", ImVec2(120, 30))) {
            on_wipe_data();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 30))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 16));
    section_header("What Echo collects - and what it never touches");
    ImGui::Columns(2, nullptr, false);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::accent());
    ImGui::TextUnformatted("Collected");
    ImGui::PopStyleColor();
    ImGui::BulletText("Foreground application name");
    ImGui::BulletText("Session start / end / duration");
    ImGui::BulletText("Application transitions");
    ImGui::BulletText("Aggregated CPU / RAM / disk / network totals");
    ImGui::BulletText("Idle / active periods");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::TextUnformatted("Never collected");
    ImGui::PopStyleColor();
    ImGui::BulletText("Keystrokes or typed content");
    ImGui::BulletText("Passwords or clipboard content");
    ImGui::BulletText("Window titles or file names");
    ImGui::BulletText("Screenshots, microphone, camera");
    ImGui::BulletText("Browser page content");
    ImGui::Columns(1);
}

// ---------- Settings ----------
void UI::draw_settings(Configuration& cfg, std::function<void()> on_config_save) {
    section_header("Settings");

    section_header("Appearance");
    {
        int theme = static_cast<int>(cfg.theme);
        const char* items[] = { "Dark", "Light", "System" };
        if (ImGui::Combo("Theme", &theme, items, 3)) {
            cfg.theme = static_cast<ThemeMode>(theme);
            if (cfg.theme == ThemeMode::Dark)  Theme::apply_dark(cfg.ui_scale);
            if (cfg.theme == ThemeMode::Light) Theme::apply_light(cfg.ui_scale);
            on_config_save();
        }
        float scale = cfg.ui_scale;
        if (ImGui::SliderFloat("UI scale", &scale, 0.8f, 1.6f, "%.2fx")) {
            cfg.ui_scale = scale;
            if (cfg.theme == ThemeMode::Light) Theme::apply_light(cfg.ui_scale);
            else Theme::apply_dark(cfg.ui_scale);
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) on_config_save();
        if (ImGui::Checkbox("Reduced motion", &cfg.reduced_motion)) on_config_save();
    }

    ImGui::Dummy(ImVec2(0, 12));
    section_header("Monitoring");
    {
        if (ImGui::Checkbox("Enable monitoring", &cfg.monitoring_enabled)) on_config_save();
        if (ImGui::Checkbox("Launch on startup", &cfg.startup_enabled)) on_config_save();
        if (ImGui::Checkbox("Process monitoring", &cfg.monitor_processes)) on_config_save();
        if (ImGui::Checkbox("System metrics", &cfg.monitor_system)) on_config_save();
        if (ImGui::Checkbox("Network metadata", &cfg.monitor_network)) on_config_save();
    }

    ImGui::Dummy(ImVec2(0, 12));
    section_header("Performance");
    {
        int si = cfg.sample_interval_sec;
        if (ImGui::SliderInt("Sample interval (s)", &si, 1, 10)) cfg.sample_interval_sec = si;
        if (ImGui::IsItemDeactivatedAfterEdit()) on_config_save();
        int it = cfg.idle_threshold_sec;
        if (ImGui::SliderInt("Idle threshold (s)", &it, 30, 600)) cfg.idle_threshold_sec = it;
        if (ImGui::IsItemDeactivatedAfterEdit()) on_config_save();
    }

    ImGui::Dummy(ImVec2(0, 12));
    section_header("About");
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_secondary());
    ImGui::TextUnformatted("Aurevix Echo  .  Version 1.0.0  .  Built with C++20");
    ImGui::TextUnformatted("Local behavioral intelligence for your computer.");
    ImGui::PopStyleColor();
}

// ---------- Query ----------
static std::string lower(std::string s) {
    for (auto& c : s) c = char(std::tolower((unsigned char)c));
    return s;
}

std::string UI::evaluate_query(const std::string& q) {
    std::string ql = lower(q);
    Timestamp today = start_of_day(now());
    Timestamp from = today, to = today + 86400;

    if (ql.find("most") != std::string::npos || ql.find("top") != std::string::npos) {
        auto apps = SessionAnalyzer::top_apps(from, to, 3);
        if (apps.empty()) return "No activity recorded today yet.";
        std::ostringstream o;
        o << "Today you spent the most time in " << apps.front().app_name
          << " (" << format_human_duration(apps.front().seconds) << ", "
          << apps.front().sessions << " sessions).";
        if (apps.size() > 1) {
            o << " Followed by " << apps[1].app_name
              << " (" << format_human_duration(apps[1].seconds) << ").";
        }
        return o.str();
    }

    if (ql.find("switch") != std::string::npos &&
        ql.find(" from ") != std::string::npos &&
        ql.find(" to ")   != std::string::npos) {
        auto pf = ql.find(" from ");
        auto pt = ql.find(" to ", pf + 6);
        std::string a = q.substr(pf + 6, pt - pf - 6);
        std::string b = q.substr(pt + 4);
        for (char c : {'.', '?', '!'}) {
            if (!b.empty() && b.back() == c) b.pop_back();
        }
        while (!a.empty() && a.front() == ' ') a.erase(a.begin());
        while (!b.empty() && b.front() == ' ') b.erase(b.begin());
        auto counts = SessionAnalyzer::transition_counts(from, to, 1000);
        for (auto& c : counts) {
            if (lower(c.from_name).find(lower(a)) != std::string::npos &&
                lower(c.to_name).find(lower(b))   != std::string::npos) {
                return "Today you moved from " + c.from_name + " to " +
                       c.to_name + " " + std::to_string(c.count) + " times.";
            }
        }
        return "No transitions matching that direction were recorded today.";
    }

    if (ql.find("longest") != std::string::npos) {
        auto l = SessionAnalyzer::longest_session(from, to);
        if (!l) return "No sessions recorded today.";
        return "Your longest session today was " + l->app_name + " for " +
               format_human_duration(l->duration_seconds) + " (started " +
               format_clock(l->started_at) + ").";
    }

    if (ql.find("before coding") != std::string::npos ||
        ql.find("before code")   != std::string::npos) {
        auto transitions = TransitionRepository::between(from, to, 2000);
        std::vector<std::string> preceding;
        for (auto& t : transitions) {
            std::string to_lower = lower(t.to_name);
            if (to_lower.find("code") != std::string::npos ||
                to_lower.find("intellij") != std::string::npos ||
                to_lower.find("visual studio") != std::string::npos ||
                to_lower.find("pycharm") != std::string::npos) {
                preceding.push_back(t.from_name);
            }
        }
        if (preceding.empty()) return "No coding sessions found today.";
        std::map<std::string, int> counts;
        for (auto& p : preceding) counts[p]++;
        std::string best; int bestn = 0;
        for (auto& kv : counts) {
            if (kv.second > bestn) { best = kv.first; bestn = kv.second; }
        }
        return "Before opening coding tools today, you most often came from " + best +
               " (" + std::to_string(bestn) + " of " +
               std::to_string(preceding.size()) + " times).";
    }

    if (ql.find("workflow") != std::string::npos || ql.find("repeat") != std::string::npos) {
        auto counts = SessionAnalyzer::transition_counts(now() - 7 * 86400, now(), 5);
        if (counts.empty()) return "No repeated transitions recorded in the last 7 days yet.";
        std::ostringstream o;
        o << "The most repeated transition over the last week was "
          << counts.front().from_name << " -> " << counts.front().to_name
          << " (" << counts.front().count << " times).";
        return o.str();
    }

    return "Echo couldn't understand that query yet.\nTry: \"What did I use most today?\"";
}

// ---------- Main render ----------
void UI::render(ActivityMonitor& monitor,
                const Configuration& cfg,
                Configuration& cfg_mutable,
                std::function<void()> on_config_save,
                std::function<void()> on_pause_monitoring,
                std::function<void()> on_resume_monitoring,
                std::function<void()> on_wipe_data) {
    if (cfg.theme == ThemeMode::System) {
        Theme::apply_dark(cfg.ui_scale);
    }

    begin_frame();

    if (onboarding_) {
        draw_onboarding(cfg_mutable, on_config_save);
        end_frame();
        return;
    }

    ImGui::BeginChild("##sidebar_container",
                      ImVec2(sidebar_collapsed_ ? 68.0f : 220.0f, 0), false);
    draw_sidebar();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##main_container", ImVec2(0, 0), false);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::bg_primary());
    ImGui::BeginChild("##main", ImVec2(0, 0), false);

    switch (page_) {
        case Page::Overview: draw_overview(monitor); break;
        case Page::Timeline: draw_timeline(); break;
        case Page::Workflow: draw_workflow(); break;
        case Page::Patterns: draw_patterns(); break;
        case Page::Replay:   draw_replay();   break;
        case Page::Insights: draw_insights(); break;
        case Page::System:   draw_system();   break;
        case Page::Privacy:  draw_privacy(cfg, cfg_mutable, on_config_save,
                                          on_pause_monitoring, on_resume_monitoring,
                                          on_wipe_data); break;
        case Page::Settings: draw_settings(cfg_mutable, on_config_save); break;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndChild();

    end_frame();
}

} // namespace aurevix