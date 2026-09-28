#include "utils/Export.h"
#include "analytics/SessionAnalyzer.h"
#include "core/Logger.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>

namespace aurevix {

std::filesystem::path Exporter::default_export_dir() {
    PWSTR raw = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &raw))) {
        base = raw; CoTaskMemFree(raw);
    } else {
        base = std::filesystem::temp_directory_path();
    }
    auto dir = base / L"AurevixEcho Exports";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

static std::string escape_json(const std::string& s) {
    std::string o; o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8]; std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o += buf;
                } else o += c;
        }
    }
    return o;
}

std::filesystem::path Exporter::export_json(Timestamp from, Timestamp to,
                                            const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    std::string name = "aurevix_echo_export_" + format_day(from) + "_" +
                       format_day(to) + ".json";
    auto path = dir / name;

    std::ofstream out(path);
    if (!out) return {};

    auto apps = SessionAnalyzer::top_apps(from, to, 1000);
    auto genome = SessionAnalyzer::genome(from, to);

    out << "{\n";
    out << "  \"generated_at\": " << now() << ",\n";
    out << "  \"window_from\": " << from << ",\n";
    out << "  \"window_to\": "   << to   << ",\n";
    out << "  \"applications\": [\n";
    for (size_t i = 0; i < apps.size(); ++i) {
        auto& a = apps[i];
        out << "    {\"application\":\"" << escape_json(a.app_name)
            << "\",\"duration_seconds\":" << a.seconds
            << ",\"sessions\":" << a.sessions << "}";
        if (i + 1 < apps.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"genome\": {\n";
    out << "    \"context_switching\": " << genome.context_switching << ",\n";
    out << "    \"session_depth\": "     << genome.session_depth     << ",\n";
    out << "    \"app_diversity\": "     << genome.app_diversity     << ",\n";
    out << "    \"repetition\": "        << genome.repetition        << ",\n";
    out << "    \"idle_ratio\": "        << genome.idle_ratio        << ",\n";
    out << "    \"workflow_stability\": "<< genome.workflow_stability<< ",\n";
    out << "    \"samples\": "           << genome.samples           << "\n";
    out << "  }\n";
    out << "}\n";
    return path;
}

std::filesystem::path Exporter::export_csv(Timestamp from, Timestamp to,
                                           const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    auto path = dir / ("aurevix_echo_export_" + format_day(from) + "_" +
                       format_day(to) + ".csv");
    std::ofstream out(path);
    if (!out) return {};

    out << "application,duration_seconds,sessions\n";
    auto apps = SessionAnalyzer::top_apps(from, to, 10000);
    for (auto& a : apps) {
        std::string n = a.app_name;
        for (char& c : n) if (c == ',' || c == '"') c = ' ';
        out << n << "," << a.seconds << "," << a.sessions << "\n";
    }
    return path;
}

} // namespace aurevix