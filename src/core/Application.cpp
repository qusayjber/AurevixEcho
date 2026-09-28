#include "core/Application.h"
#include "core/Logger.h"
#include "core/TimeManager.h"
#include "storage/Database.h"
#include "storage/Repositories.h"
#include "ui/Theme.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <string>

#include <imgui.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace aurevix {

static std::filesystem::path data_dir() {
    PWSTR raw = nullptr;
    std::filesystem::path p;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw))) {
        p = raw; CoTaskMemFree(raw);
    } else {
        p = std::filesystem::temp_directory_path();
    }
    p /= L"AurevixEcho";
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return p;
}

Application::Application() = default;

Application::~Application() {
    stop_maintenance();
    monitor_.stop();
    Database::instance().close();
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool Application::initialize() {
    load_config();

    // ---- Open database ----
    auto db_path = data_dir() / L"echo.db";
    if (!Database::instance().open(db_path)) {
        MessageBoxW(nullptr,
                    L"Failed to open the local database.",
                    L"Aurevix Echo", MB_ICONERROR);
        return false;
    }
    Database::instance().migrate();
    apply_retention();

    // ---- Create window ----
    if (!create_main_window()) {
        MessageBoxW(nullptr,
                    L"Failed to create the main window.",
                    L"Aurevix Echo", MB_ICONERROR);
        return false;
    }

    // ---- Graphics ----
    if (!ui_.create(hwnd_)) {
        MessageBoxW(nullptr,
                    L"Failed to initialize the graphics context.",
                    L"Aurevix Echo", MB_ICONERROR);
        return false;
    }

    // Push onboarding state through the public UI interface.
    ui_.set_onboarding(!cfg_.first_run_completed);
    ui_.set_selected_day(start_of_day(now()));

    // Start monitoring if the user previously consented.
    if (cfg_.monitoring_enabled && cfg_.first_run_completed) {
        monitor_.start(cfg_);
    }

    // ---- Background maintenance thread ----
    // Important: mark initialized BEFORE creating the thread, so the
    // thread's while-loop condition is immediately true.
    initialized_.store(true);
    maintenance_thread_ = std::thread([this] {
        while (initialized_.load()) {
            // Sleep in 1-second slices so we react quickly to a shutdown.
            for (int i = 0; i < 15 * 60 && initialized_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!initialized_.load()) break;
            apply_retention();
        }
    });

    return true;
}

void Application::stop_maintenance() {
    // exchange returns the PREVIOUS value. If it was true, we still own the
    // thread and must join it. If it was false, someone already stopped it.
    bool was_running = initialized_.exchange(false);
    if (was_running && maintenance_thread_.joinable()) {
        maintenance_thread_.join();
    }
}

void Application::load_config() {
    auto path = data_dir() / L"settings.cfg";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) return;

    std::ifstream in(path);
    if (!in) return;
    std::stringstream ss;
    ss << in.rdbuf();
    cfg_.load(ss.str());
}

void Application::save_config() {
    auto path = data_dir() / L"settings.cfg";
    std::ofstream out(path);
    if (!out) return;
    out << cfg_.save();
    install_autostart(cfg_.startup_enabled);
}

void Application::apply_retention() {
    if (cfg_.retention_days <= 0) return;
    Timestamp cutoff = now() - std::int64_t(cfg_.retention_days) * 86400;
    SettingsRepository::remove_old_data(cutoff);
    SystemMetricsRepository::prune_before(cutoff);
}

bool Application::create_main_window() {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = &Application::wnd_proc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"AurevixEchoWindowClass";

    if (!RegisterClassExW(&wc)) {
        // Class may already be registered if initialize() is called twice;
        // that is not fatal. Continue.
    }

    hwnd_ = CreateWindowExW(
        0, wc.lpszClassName, L"Aurevix Echo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1320, 860,
        nullptr, nullptr, wc.hInstance, this);

    if (!hwnd_) return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

LRESULT CALLBACK Application::wnd_proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w, l)) return true;

    Application* self = reinterpret_cast<Application*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(l);
        self = reinterpret_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(self));
    }

    if (self) return self->handle_message(hwnd, msg, w, l);
    return DefWindowProcW(hwnd, msg, w, l);
}

LRESULT Application::handle_message(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
        case WM_SIZE:
            // Route through the public UI interface — never touch DX members.
            ui_.on_resize(static_cast<unsigned>(LOWORD(l)),
                          static_cast<unsigned>(HIWORD(l)));
            return 0;

        case WM_SYSCOMMAND:
            // Disable the Alt-key menu activation beep.
            if ((w & 0xfff0) == SC_KEYMENU) return 0;
            break;

        case WM_CLOSE:
            // Stop background work BEFORE tearing anything down.
            stop_maintenance();
            save_config();
            PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            stop_maintenance();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

void Application::install_autostart(bool enable) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return;
    }

    if (enable) {
        wchar_t path[MAX_PATH]{};
        DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            RegSetValueExW(
                key, L"AurevixEcho", 0, REG_SZ,
                reinterpret_cast<const BYTE*>(path),
                static_cast<DWORD>((wcslen(path) + 1) * sizeof(wchar_t)));
        }
    } else {
        RegDeleteValueW(key, L"AurevixEcho");
    }

    RegCloseKey(key);
}

int Application::run() {
    bool running = true;
    MSG msg{};

    while (running && initialized_.load()) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
        }
        if (!running) break;

        auto save_cb = [this] {
            save_config();
            monitor_.update_configuration(cfg_);
        };
        auto pause_cb = [this] {
            monitor_.stop();
        };
        auto resume_cb = [this] {
            cfg_.monitoring_enabled = true;
            monitor_.start(cfg_);
        };
        auto wipe_cb = [this] {
            Database::instance().wipe_user_data();
        };

        ui_.render(monitor_, cfg_, cfg_,
                   save_cb, pause_cb, resume_cb, wipe_cb);
    }

    // Ensure background threads stop before we return.
    stop_maintenance();
    save_config();
    return 0;
}

} // namespace aurevix