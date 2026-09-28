#pragma once
#include "core/Configuration.h"
#include "monitoring/ActivityMonitor.h"
#include "graph/WorkflowGraph.h"
#include "analytics/PatternDetector.h"
#include "storage/Repositories.h"

#include <filesystem>
#include <string>
#include <vector>
#include <functional>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct HWND__;
using HWND = HWND__*;

namespace aurevix {

class UI {
public:
    UI();
    ~UI();

    bool create(HWND hwnd);
    void destroy();
    void render(ActivityMonitor& monitor,
                const Configuration& cfg,
                Configuration& cfg_mutable,
                std::function<void()> on_config_save,
                std::function<void()> on_pause_monitoring,
                std::function<void()> on_resume_monitoring,
                std::function<void()> on_wipe_data);

    bool wants_close() const { return wants_close_; }

    // ---- Interfaces used by Application ----
    void set_onboarding(bool v)        { onboarding_ = v; }
    void set_selected_day(Timestamp t) { selected_day_ = t; }
    void on_resize(unsigned w, unsigned h);

private:
    enum class Page { Overview, Timeline, Workflow, Patterns, Replay, Insights,
                      System, Privacy, Settings };
    enum class OnboardingStep { Welcome, Privacy, WhatIsCollected, Configure, Done };

    void begin_frame();
    void end_frame();
    void draw_sidebar();
    void draw_onboarding(Configuration& cfg,
                         std::function<void()> on_config_save);
    void draw_overview(ActivityMonitor& monitor);
    void draw_timeline();
    void draw_workflow();
    void draw_patterns();
    void draw_replay();
    void draw_insights();
    void draw_system();
    void draw_privacy(const Configuration& cfg,
                      Configuration& cfg_mutable,
                      std::function<void()> on_config_save,
                      std::function<void()> on_pause_monitoring,
                      std::function<void()> on_resume_monitoring,
                      std::function<void()> on_wipe_data);
    void draw_settings(Configuration& cfg, std::function<void()> on_config_save);

    // helpers
    void card(const char* title, const char* value, const char* subtitle = "");
    void section_header(const char* title, const char* subtitle = "");
    void metric_bar(const char* label, float value_0_to_1, const char* hint = "");
    void draw_timeline_ribbon(Timestamp dayStart, Timestamp dayEnd,
                              const std::vector<SessionRow>& sessions);
    void draw_flow_ribbon(const std::vector<std::string>& flow);

    // state
    Page page_ = Page::Overview;
    bool onboarding_ = true;
    OnboardingStep onb_step_ = OnboardingStep::Welcome;

    bool wants_close_ = false;
    bool sidebar_collapsed_ = false;

    // Timeline selection
    Timestamp selected_day_ = 0;

    // Replay state
    bool replay_playing_ = false;
    float replay_time_ = 0.0f; // 0..1
    float replay_speed_ = 1.0f;

    // Workflow
    WorkflowGraph wgraph_;
    Timestamp wgraph_window_ = 0; // last build timestamp
    std::string wgraph_filter_;

    // Patterns
    std::vector<DetectedPattern> patterns_;
    Timestamp patterns_built_ = 0;

    // Insights
    std::vector<DetectedPattern> insights_;
    Timestamp insights_built_ = 0;

    // Query interface
    std::string query_text_;
    std::string query_result_;

    // system metrics sample
    Timestamp last_system_fetch_ = 0;
    std::vector<SystemMetricRow> system_series_;

    // transition cache
    std::vector<TransitionRow> transitions_cache_;
    Timestamp transitions_cache_time_ = 0;

    // Windows / DX
    HWND                       hwnd_ = nullptr;
    ID3D11Device*              device_ = nullptr;
    ID3D11DeviceContext*       ctx_ = nullptr;
    IDXGISwapChain*            swap_ = nullptr;
    ID3D11RenderTargetView*    rtv_ = nullptr;
    bool                       dx_ok_ = false;

    bool create_device_d3d(HWND hwnd);
    void destroy_device_d3d();
    void create_rtv();
    void destroy_rtv();

    // query eval
    std::string evaluate_query(const std::string& q);
};

} // namespace aurevix