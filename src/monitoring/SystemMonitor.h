#pragma once
#include "storage/Repositories.h"
#include <cstdint>

namespace aurevix {

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    SystemMetricRow sample();

private:
    // CPU differential state
    unsigned long long prev_idle_   = 0;
    unsigned long long prev_kernel_ = 0;
    unsigned long long prev_user_   = 0;

    // Disk PDH state
    void* disk_query_   = nullptr;
    void* disk_counter_ = nullptr;

    // Network state
    unsigned long long prev_net_in_   = 0;
    unsigned long long prev_net_out_  = 0;
    unsigned long long prev_net_tick_ = 0;

    double sample_cpu();
    double sample_ram();
    double sample_disk();
    double sample_network_kbps();
    long   sample_process_count();
};

} // namespace aurevix