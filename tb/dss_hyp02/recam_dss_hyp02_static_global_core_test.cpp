#include "Vrecam_dss_hyp02_static_global_core.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

struct Path {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
};

void tick(Vrecam_dss_hyp02_static_global_core& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

bool legal(const Path& path) {
    return ((path.c & 2U) == 0U || (path.a & 1U) != 0U) &&
           ((path.a & 2U) == 0U || (path.b & 1U) != 0U) &&
           ((path.b & 2U) == 0U || (path.d & 1U) != 0U) &&
           ((path.d & 2U) == 0U || (path.c & 1U) != 0U);
}

std::array<Path, 81> static_paths() {
    std::array<Path, 81> paths{};
    unsigned index = 0U;
    for (unsigned d = 0U; d < 4U; ++d) {
        for (unsigned c = 0U; c < 4U; ++c) {
            for (unsigned b = 0U; b < 4U; ++b) {
                for (unsigned a = 0U; a < 4U; ++a) {
                    const Path path{a, b, c, d};
                    if (legal(path)) {
                        paths.at(index++) = path;
                    }
                }
            }
        }
    }
    return paths;
}

unsigned config_for_slot(unsigned sa, unsigned slot) {
    if (sa == 0U || sa == 3U) {
        static constexpr unsigned kConfig[] = {0U, 4U, 5U, 6U};
        return kConfig[slot];
    }
    return slot;
}

unsigned pattern_for(unsigned sa, unsigned slot) {
    return 1U + sa * 4U + slot;
}

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void reset(Vrecam_dss_hyp02_static_global_core& dut) {
    dut.start_i = 0;
    dut.candidate_valid_i = 0;
    dut.candidate_pattern_id_i = 0;
    dut.rst_ni = 0;
    tick(dut);
    dut.rst_ni = 1;
    tick(dut);
}

void run_case(Vrecam_dss_hyp02_static_global_core& dut, const Path* path) {
    reset(dut);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;

    for (unsigned count = 0U; count < 16U; ++count) {
        dut.eval();
        require(dut.collection_active_o != 0U, "collection did not retain its cycle boundary");
        const unsigned sa = dut.current_sa_o;
        const unsigned slot = dut.current_slot_o;
        require(sa == count / 4U && slot == count % 4U, "collection order changed");
        const bool selected = path != nullptr &&
                              ((sa == 0U && slot == path->a) ||
                               (sa == 1U && slot == path->b) ||
                               (sa == 2U && slot == path->c) ||
                               (sa == 3U && slot == path->d));
        dut.candidate_valid_i = selected ? 1U : 0U;
        dut.candidate_pattern_id_i = pattern_for(sa, slot);
        tick(dut);
    }

    dut.eval();
    require(dut.allocation_active_o != 0U, "decision cycle missing");
    tick(dut);
    dut.eval();
    require(dut.done_o != 0U, "completion pulse missing");

    if (path == nullptr) {
        require(dut.group_repairable_o == 0U, "empty candidate map repaired");
        require(dut.sa_commit_valid_o == 0U, "empty candidate map committed");
        return;
    }

    require(dut.group_repairable_o != 0U, "legal path was not committed");
    require(dut.sa_commit_valid_o == 0xfU, "legal path did not atomically commit all SAs");
    const unsigned expected_configs = config_for_slot(0U, path->a) |
                                      (config_for_slot(1U, path->b) << 3U) |
                                      (config_for_slot(2U, path->c) << 6U) |
                                      (config_for_slot(3U, path->d) << 9U);
    const unsigned expected_patterns = (pattern_for(0U, path->a) & 0xfU) |
                                       ((pattern_for(1U, path->b) & 0xfU) << 4U) |
                                       ((pattern_for(2U, path->c) & 0xfU) << 8U) |
                                       ((pattern_for(3U, path->d) & 0xfU) << 12U);
    const unsigned expected_release = ((path->c & 1U) << 3U) |
                                      ((path->b & 1U) << 2U) |
                                      ((path->d & 1U) << 1U) |
                                      (path->a & 1U);
    const unsigned expected_borrow = (((path->d >> 1U) & 1U) << 3U) |
                                     (((path->c >> 1U) & 1U) << 2U) |
                                     (((path->b >> 1U) & 1U) << 1U) |
                                     ((path->a >> 1U) & 1U);
    unsigned expected_ledger = expected_release;
    expected_ledger |= ((path->c >> 1U) & 1U) << 5U;
    expected_ledger |= ((path->b >> 1U) & 1U) << 6U;
    expected_ledger |= ((path->a >> 1U) & 1U) << 8U;
    expected_ledger |= ((path->d >> 1U) & 1U) << 11U;
    require(dut.selected_config_flat_o == expected_configs, "config decode changed");
    require(dut.selected_pattern_flat_o == expected_patterns, "pattern mux changed");
    require(dut.release_flat_o == expected_release, "release metadata changed");
    require(dut.borrow_flat_o == expected_borrow, "borrow metadata changed");
    require(dut.selected_donor_flat_o == 0xc6U, "fixed directional donor metadata changed");
    require(dut.ledger_released_borrower_o == expected_ledger, "legacy diagnostic ledger changed");
}

}  // namespace

int main() {
    const auto paths = static_paths();
    Vrecam_dss_hyp02_static_global_core dut;
    const std::array<unsigned, 10> directed_ids = {0U, 1U, 4U, 11U, 29U, 41U, 53U, 65U, 74U, 80U};
    run_case(dut, nullptr);
    for (const unsigned id : directed_ids) {
        run_case(dut, &paths.at(id));
    }
    std::cout << "CORE_DIRECTED_PATHS_CHECKED: 10\n";
    std::cout << "CORE_EMPTY_MAP_CHECKED: 1\n";
    std::cout << "CORE_ATOMIC_COMMIT_ERRORS: 0\n";
    return 0;
}
