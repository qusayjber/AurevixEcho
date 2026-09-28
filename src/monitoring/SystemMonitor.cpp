// =====================================================================
//  Aurevix Echo — SystemMonitor.cpp
//  Aggregated system metrics only. No content is collected.
//  Uses iphlpapi GetIfTable (portable across all Windows SDKs).
// =====================================================================

#include "monitoring/SystemMonitor.h"
#include "core/TimeManager.h"
#include "core/Logger.h"

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iphlpapi.h>
#include <psapi.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace aurevix {

SystemMonitor::SystemMonitor() {
    // Initialize PDH disk counter (PhysicalDisk % Disk Time, aggregated).
    PDH_HQUERY   q = nullptr;
    PDH_HCOUNTER c = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &q) == ERROR_SUCCESS &&
        PdhAddEnglishCounterW(q, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &c) == ERROR_SUCCESS) {
        PdhCollectQueryData(q);
        disk_query_   = q;
        disk_counter_ = c;
    } else {
        if (q) PdhCloseQuery(q);
        AUR_WARN("PDH disk counter unavailable; disk metrics will report 0.");
    }
}

SystemMonitor::~SystemMonitor() {
    if (disk_query_) {
        PdhCloseQuery(static_cast<PDH_HQUERY>(disk_query_));
        disk_query_   = nullptr;
        disk_counter_ = nullptr;
    }
}

// ---------------------------------------------------------------------
//  CPU — differential across samples.
// ---------------------------------------------------------------------
double SystemMonitor::sample_cpu() {
    FILETIME idle{}, kernel{}, user{};
    if (!GetSystemTimes(&idle, &kernel, &user)) return 0.0;

    auto to_ull = [](const FILETIME& ft) -> unsigned long long {
        return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32)
             |  static_cast<unsigned long long>(ft.dwLowDateTime);
    };

    const unsigned long long i = to_ull(idle);
    const unsigned long long k = to_ull(kernel);
    const unsigned long long u = to_ull(user);

    const unsigned long long di = i - prev_idle_;
    const unsigned long long dk = k - prev_kernel_;
    const unsigned long long du = u - prev_user_;

    prev_idle_   = i;
    prev_kernel_ = k;
    prev_user_   = u;

    // kernel time includes idle time.
    const unsigned long long total = dk + du;
    if (total == 0) return 0.0;

    const double busy = double(total - di);
    double pct = 100.0 * busy / double(total);
    if (pct < 0.0)   pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    return pct;
}

// ---------------------------------------------------------------------
//  RAM — dwMemoryLoad is a direct percentage.
// ---------------------------------------------------------------------
double SystemMonitor::sample_ram() {
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return 0.0;
    return double(ms.dwMemoryLoad);
}

// ---------------------------------------------------------------------
//  Disk — PDH aggregate counter.
// ---------------------------------------------------------------------
double SystemMonitor::sample_disk() {
    if (!disk_query_ || !disk_counter_) return 0.0;

    PDH_HQUERY   q = static_cast<PDH_HQUERY>(disk_query_);
    PDH_HCOUNTER c = static_cast<PDH_HCOUNTER>(disk_counter_);

    PdhCollectQueryData(q);

    PDH_FMT_COUNTERVALUE v{};
    if (PdhGetFormattedCounterValue(c, PDH_FMT_DOUBLE, nullptr, &v) == ERROR_SUCCESS) {
        double pct = v.doubleValue;
        if (pct < 0.0)   pct = 0.0;
        if (pct > 100.0) pct = 100.0;
        return pct;
    }
    return 0.0;
}

// ---------------------------------------------------------------------
//  Network — GetIfTable (MIB_IFTABLE / MIB_IFROW), available everywhere.
//
//  MIB_IFROW counters are 32-bit. On multi-gigabit links they can wrap
//  between samples. We detect the wrap by a "curr < prev" condition and
//  clamp the rate to 0 for that interval — no garbage spike is shown.
// ---------------------------------------------------------------------
double SystemMonitor::sample_network_kbps() {
    // First call: query required size.
    DWORD size = 0;
    if (GetIfTable(nullptr, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return 0.0;
    }

    MIB_IFTABLE* table = static_cast<MIB_IFTABLE*>(std::malloc(size));
    if (!table) return 0.0;

    double kbps = 0.0;
    if (GetIfTable(table, &size, FALSE) == NO_ERROR) {
        unsigned long long total_in  = 0;
        unsigned long long total_out = 0;

        for (DWORD idx = 0; idx < table->dwNumEntries; ++idx) {
            const MIB_IFROW& row = table->table[idx];

            // Skip loopback.
            if (row.dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

            // Skip interfaces that are not operationally up.
            if (row.dwOperStatus != IF_OPER_STATUS_OPERATIONAL) continue;

            total_in  += row.dwInOctets;
            total_out += row.dwOutOctets;
        }

        const unsigned long long tick = GetTickCount64();
        if (prev_net_tick_ != 0 && tick > prev_net_tick_) {
            const double dt = double(tick - prev_net_tick_) / 1000.0; // seconds
            const unsigned long long prev_total = prev_net_in_ + prev_net_out_;
            const unsigned long long curr_total = total_in + total_out;

            if (curr_total >= prev_total) {
                const unsigned long long delta = curr_total - prev_total;
                kbps = (double(delta) / 1024.0) / dt;   // KiB/s
            }
            // else: counters wrapped, report 0 for this interval.
        }

        prev_net_in_   = total_in;
        prev_net_out_  = total_out;
        prev_net_tick_ = tick;
    }

    std::free(table);
    return kbps;
}

// ---------------------------------------------------------------------
//  Process count — one-shot enumeration.
// ---------------------------------------------------------------------
long SystemMonitor::sample_process_count() {
    DWORD pids[2048];
    DWORD needed = 0;
    if (EnumProcesses(pids, sizeof(pids), &needed)) {
        return static_cast<long>(needed / sizeof(DWORD));
    }
    return 0;
}

// ---------------------------------------------------------------------
//  Full sample.
// ---------------------------------------------------------------------
SystemMetricRow SystemMonitor::sample() {
    SystemMetricRow m;
    m.at            = now();
    m.cpu_pct       = sample_cpu();
    m.ram_pct       = sample_ram();
    m.disk_pct      = sample_disk();
    m.net_kbps      = sample_network_kbps();
    m.process_count = sample_process_count();
    return m;
}

} // namespace aurevix