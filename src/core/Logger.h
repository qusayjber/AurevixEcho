#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace aurevix {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static Logger& instance();

    void init();
    void log(LogLevel level, const std::string& msg);

private:
    Logger() = default;
    std::mutex mutex_;
    std::ofstream file_;
    bool enabled_ = false;
};

#define AUR_LOG(level, expr) do { \
    std::ostringstream _aur_s; _aur_s << expr; \
    ::aurevix::Logger::instance().log(level, _aur_s.str()); \
} while (0)

#define AUR_DEBUG(expr) AUR_LOG(::aurevix::LogLevel::Debug, expr)
#define AUR_INFO(expr)  AUR_LOG(::aurevix::LogLevel::Info,  expr)
#define AUR_WARN(expr)  AUR_LOG(::aurevix::LogLevel::Warn,  expr)
#define AUR_ERROR(expr) AUR_LOG(::aurevix::LogLevel::Error, expr)

} // namespace aurevix