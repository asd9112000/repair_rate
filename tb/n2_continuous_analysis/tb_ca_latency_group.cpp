#define STR2(x) #x
#define STR(x) STR2(x)
#include STR(CA_LATENCY_HEADER)

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

using D = CA_LATENCY_DUT;

static long cyc = 0;

static void tick(D &d) {
    d.clk_i = 0;
    d.eval();
    d.clk_i = 1;
    d.eval();
    d.clk_i = 0;
    d.eval();
    ++cyc;
}

static void reset(D &d) {
    d.rst_ni = 0;
    d.canonical_start_i = 0;
    d.state_update_i = 0;
    d.test_done_valid_i = 0;
    d.seeds_i = 0;
    tick(d);
    tick(d);
    d.rst_ni = 1;
}

static uint32_t nx(uint32_t &x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static bool same(const D &d) {
    if (d.canonical_repairable_o != d.live_repairable_o ||
        d.canonical_config_o != d.live_config_o ||
        d.canonical_pattern_o != d.live_pattern_o ||
        d.canonical_donor_o != d.live_donor_o ||
        d.canonical_borrow_o != d.live_borrow_o ||
        d.canonical_release_o != d.live_release_o ||
        d.canonical_is_row_o != d.live_is_row_o ||
        d.canonical_line_valid_o != d.live_line_valid_o)
        return false;
    for (int i = 0; i < 9; ++i)
        if (d.canonical_address_o[i] != d.live_address_o[i]) return false;
    return true;
}

int main() {
    uint32_t x = 0x20260928;
    D d;
    std::vector<int> latency;

    for (int v = 0; v < 10000; ++v) {
        reset(d);
        d.seeds_i = nx(x);
        d.canonical_start_i = 1;
        tick(d);
        d.canonical_start_i = 0;

        bool canonical_done = false;
        for (int i = 0; i < 24 && !canonical_done; ++i) {
            tick(d);
            canonical_done = d.canonical_done_o;
        }
        if (!canonical_done) return 1;

        long update_final_sa_cycle = 0;
        for (unsigned sa = 0; sa < 4; ++sa) {
            const bool done_first = (v ^ sa) & 1;
            d.state_update_i = 1;
            d.state_sa_i = sa;
            d.test_done_valid_i = done_first;
            d.test_done_sa_i = sa;
            tick(d);
            if (sa == 3) update_final_sa_cycle = cyc - 1;
            d.state_update_i = 0;
            d.test_done_valid_i = 0;

            for (int i = 0; i < 8 && ((d.live_frozen_o >> sa) & 1) == 0;
                 ++i) {
                if (!done_first && !d.live_scan_active_o) {
                    d.test_done_valid_i = 1;
                    d.test_done_sa_i = sa;
                }
                tick(d);
                d.test_done_valid_i = 0;
            }
            if (((d.live_frozen_o >> sa) & 1) == 0) return 1;
        }

        for (int i = 0; i < 4 && !d.live_ready_o; ++i) tick(d);
        if (!d.live_ready_o || !same(d)) {
            std::cerr << "MISMATCH=" << v << '\n';
            return 1;
        }
        latency.push_back(int(cyc - update_final_sa_cycle));
    }

    std::sort(latency.begin(), latency.end());
    long sum = 0;
    unsigned latency_5_count = 0;
    unsigned latency_6_count = 0;
    unsigned other_latency_count = 0;
    for (int cycles : latency) {
        sum += cycles;
        if (cycles == 5)
            ++latency_5_count;
        else if (cycles == 6)
            ++latency_6_count;
        else
            ++other_latency_count;
    }

    const auto q = [&](double p) { return latency[size_t((latency.size() - 1) * p)]; };
    if (latency_5_count + latency_6_count != 10000 || other_latency_count != 0) {
        std::cerr << "GROUP_LATENCY_HISTOGRAM_FAIL latency5=" << latency_5_count
                  << " latency6=" << latency_6_count
                  << " other=" << other_latency_count << '\n';
        return 1;
    }

    std::cout << "GROUP_LATENCY_REPLAY_PASS vectors=10000"
              << " corpus_seed=0x20260928 samples=" << latency.size()
              << " latency5=" << latency_5_count
              << " latency6=" << latency_6_count
              << " latency5_fraction=" << double(latency_5_count) / latency.size()
              << " latency6_fraction=" << double(latency_6_count) / latency.size()
              << " min=" << latency.front()
              << " mean=" << double(sum) / latency.size()
              << " median=" << q(.5)
              << " p95=" << q(.95)
              << " max=" << latency.back()
              << " mismatches=0\n";
}
