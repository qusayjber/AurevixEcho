#include "monitoring/WindowMonitor.h"
#include "core/Logger.h"

#include <windows.h>
#include <psapi.h>
#include <unordered_map>

namespace aurevix {

static std::string basename_noext(const std::string& p) {
    auto pos = p.find_last_of("\\/");
    std::string f = (pos == std::string::npos) ? p : p.substr(pos + 1);
    auto dot = f.find_last_of('.');
    if (dot != std::string::npos) f = f.substr(0, dot);
    return f;
}

static std::string friendly_name(const std::string& exe) {
    // Small curated mapping for common applications — no network lookups.
    static const std::unordered_map<std::string, std::string> kMap = {
        {"chrome", "Google Chrome"},
        {"msedge", "Microsoft Edge"},
        {"firefox", "Mozilla Firefox"},
        {"Code", "Visual Studio Code"},
        {"devenv", "Visual Studio"},
        {"idea64", "IntelliJ IDEA"},
        {"pycharm64", "PyCharm"},
        {"clion64", "CLion"},
        {"rider64", "Rider"},
        {"WebStorm", "WebStorm"},
        {"explorer", "File Explorer"},
        {"WindowsTerminal", "Windows Terminal"},
        {"powershell", "PowerShell"},
        {"cmd", "Command Prompt"},
        {"wt", "Windows Terminal"},
        {"notepad", "Notepad"},
        {"notepad++", "Notepad++"},
        {"sublime_text", "Sublime Text"},
        {"slack", "Slack"},
        {"Discord", "Discord"},
        {"Teams", "Microsoft Teams"},
        {"zoom", "Zoom"},
        {"Spotify", "Spotify"},
        {"vlc", "VLC"},
        {"obs64", "OBS Studio"},
        {"Steam", "Steam"},
        {"git-bash", "Git Bash"},
        {"GitHubDesktop", "GitHub Desktop"},
        {"Figma", "Figma"},
        {"Photoshop", "Adobe Photoshop"},
        {"Illustrator", "Adobe Illustrator"},
        {"blender", "Blender"},
        {"calc", "Calculator"},
        {"mspaint", "Paint"},
        {"Taskmgr", "Task Manager"},
        {"SystemSettings", "Settings"},
    };
    auto it = kMap.find(exe);
    return it != kMap.end() ? it->second : exe;
}

ForegroundInfo WindowMonitor::current_foreground() {
    ForegroundInfo info;
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return info;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return info;

    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return info;

    wchar_t buf[MAX_PATH]{};
    DWORD size = MAX_PATH;
    std::string exe;
    if (QueryFullProcessImageNameW(h, 0, buf, &size)) {
        // Convert to UTF-8
        int len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
        if (len > 1) {
            exe.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, exe.data(), len, nullptr, nullptr);
        }
    }
    CloseHandle(h);

    if (exe.empty()) return info;

    auto pos = exe.find_last_of("\\/");
    std::string file = (pos == std::string::npos) ? exe : exe.substr(pos + 1);
    auto dot = file.find_last_of('.');
    if (dot != std::string::npos) file = file.substr(0, dot);

    info.valid = true;
    info.pid = pid;
    info.process_name = file;
    info.display_name = friendly_name(file);
    return info;
}

std::uint32_t WindowMonitor::seconds_since_last_input() {
    LASTINPUTINFO li{};
    li.cbSize = sizeof(li);
    if (!GetLastInputInfo(&li)) return 0;
    DWORD tick = GetTickCount();
    DWORD diff = tick - li.dwTime;
    return diff / 1000;
}

} // namespace aurevix