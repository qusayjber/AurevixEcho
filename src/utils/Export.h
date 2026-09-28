#pragma once
#include "core/TimeManager.h"
#include <filesystem>

namespace aurevix {

class Exporter {
public:
    // Exports anonymized analytics to JSON in the given directory.
    // Returns the full path written, or empty on failure.
    static std::filesystem::path export_json(Timestamp from, Timestamp to,
                                             const std::filesystem::path& dir);

    // Exports a CSV (applications + duration + sessions).
    static std::filesystem::path export_csv(Timestamp from, Timestamp to,
                                            const std::filesystem::path& dir);

    static std::filesystem::path default_export_dir();
};

} // namespace aurevix