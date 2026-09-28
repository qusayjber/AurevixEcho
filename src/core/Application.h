#pragma once
#include "core/Configuration.h"
#include "monitoring/ActivityMonitor.h"
#include "ui/UI.h"

#include <windows.h>

#include <atomic>
#include <thread>

namespace aurevix {

class Application {
public:
    Application();
    ~Application();

    // Non-copyable, non-movable (owns HWND and worker thread)
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    bool initialize();
    int  run();

private:
    HWND              hwnd_ = nullptr;
    Configuration     cfg_;
    ActivityMonitor   monitor_;
    UI                ui_;
    std::atomic<bool> initialized_{false};
    std::thread       maintenance_thread_;

    void load_config();
    void save_config();
    void apply_retention();

    static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle_message(HWND, UINT, WPARAM, LPARAM);

    bool create_main_window();
    void install_autostart(bool enable);
    void stop_maintenance();
};

} // namespace aurevix