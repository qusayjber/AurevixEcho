#include "core/Logger.h"
#include "core/TimeManager.h"
#include <windows.h>
#include <shlobj.h>

namespace aurevix {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

static std::filesystem::path data_dir() {
    PWSTR raw = nullptr;
    std::filesystem::path p;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw))) {
        p = raw;
        CoTaskMemFree(raw);
    } else {
        p = std::filesystem::temp_directory_path();
    }
    p /= L"AurevixEcho";
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return p;
}

void Logger::init() {
    std::lock_guard lock(mutex_);
    auto path = data_dir() / L"echo.log";
    file_.open(path, std::ios::app);
    enabled_ = file_.is_open();
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard lock(mutex_);
    const char* tag = "INFO ";
    switch (level) {
        case LogLevel::Debug: tag = "DEBUG"; break;
        case LogLevel::Info:  tag = "INFO "; break;
        case LogLevel::Warn:  tag = "WARN "; break;
        case LogLevel::Error: tag = "ERROR"; break;
    }
    std::string line = "[" + format_day(now()) + " " + format_clock(now()) + "] " +
                       tag + " " + msg + "\n";
    OutputDebugStringA(line.c_str());
    if (enabled_) { file_ << line; file_.flush(); }
}

} // namespace aurevix