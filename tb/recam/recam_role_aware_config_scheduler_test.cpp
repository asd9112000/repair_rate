#include "Vrecam_role_aware_config_scheduler.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr std::array<unsigned, 4> kAd{{0, 4, 5, 6}};
constexpr std::array<unsigned, 4> kBc{{0, 2, 1, 3}};

[[noreturn]] void fail(const std::string &message) { throw std::runtime_error(message); }
void require(bool condition, const std::string &message) { if (!condition) fail(message); }

void tick(Vrecam_role_aware_config_scheduler &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

std::uint32_t entry(const Vrecam_role_aware_config_scheduler &dut, unsigned index)
{
    const unsigned bit = 20U * index;
    const unsigned word = bit / 32U;
    const unsigned offset = bit % 32U;
    const std::uint64_t words = static_cast<std::uint64_t>(dut.config_pattern_map_o[word]) |
        (static_cast<std::uint64_t>(dut.config_pattern_map_o[word + 1U]) << 32U);
    return static_cast<std::uint32_t>((words >> offset) & 0xfffffU);
}

void driveStableState(Vrecam_role_aware_config_scheduler &dut)
{
    dut.pivot_valid_i = 0x01;
    dut.pivot_rows_flat_i = 0x101;
    dut.pivot_cols_flat_i = 0x03;
    dut.row_gt1_i = 0;
    dut.row_gt2_i = 0;
    dut.row_gt3_i = 0;
    dut.col_gt1_i = 0;
    dut.col_gt2_i = 0;
    dut.col_gt3_i = 0;
    dut.hybrid_valid_i = 0;
    dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    dut.hybrid_differing_flat_i = 0;
    dut.conventional_overflow_i = 0;
}

void driveMixedSearchState(Vrecam_role_aware_config_scheduler &dut, unsigned seed)
{
    const unsigned pivots = 1U + (seed % 5U);
    dut.pivot_valid_i = (1U << pivots) - 1U;
    dut.pivot_rows_flat_i = 0x1010200801ULL;
    dut.pivot_cols_flat_i = 0x1041041U;
    dut.row_gt1_i = (seed * 3U) & 0x1fU;
    dut.row_gt2_i = (seed * 5U) & 0x1fU;
    dut.row_gt3_i = (seed * 7U) & 0x1fU;
    dut.col_gt1_i = (seed * 11U) & 0x1fU;
    dut.col_gt2_i = (seed * 13U) & 0x1fU;
    dut.col_gt3_i = (seed * 17U) & 0x1fU;
    dut.hybrid_valid_i = 0;
    dut.hybrid_pointer_flat_i = 0;
    dut.hybrid_descriptor_i = 0;
    dut.hybrid_differing_flat_i = 0;
    dut.conventional_overflow_i = 0;
}

std::uint32_t preview(const Vrecam_role_aware_config_scheduler &dut)
{
    return (static_cast<std::uint32_t>(dut.current_candidate_valid_o) << 10U) |
           (static_cast<std::uint32_t>(dut.current_pattern_id_o) << 6U) |
           (static_cast<std::uint32_t>(dut.current_repairable_o) << 5U) |
           (static_cast<std::uint32_t>(dut.current_solution_valid_o) << 4U) |
           (static_cast<std::uint32_t>(dut.current_config_id_o) << 1U) | 1U;
}

void reset(Vrecam_role_aware_config_scheduler &dut)
{
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.sa_role_i = 0;
    driveStableState(dut);
    tick(dut);
    dut.rst_ni = 1;
    require(!dut.busy_o && !dut.done_o && dut.config_pattern_map_o[0] == 0 &&
                dut.config_pattern_map_o[1] == 0 && dut.config_pattern_map_o[2] == 0,
            "reset did not clear scheduler state");
}

void scanAndCheck(Vrecam_role_aware_config_scheduler &dut,
                  bool roleBc, const std::array<unsigned, 4> &expected,
                  bool assertBusyStart)
{
    dut.sa_role_i = roleBc;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    require(dut.busy_o && !dut.done_o, "start was not accepted");

    for (unsigned i = 0; i < 4; ++i) {
        require(dut.current_config_id_o == expected[i], "incorrect emitted ConfigID");
        const std::uint32_t expectedEntry = preview(dut);
        if (assertBusyStart && i == 1) {
            dut.sa_role_i = !roleBc;
            dut.start_i = 1;
        }
        tick(dut);
        dut.start_i = 0;
        require(entry(dut, i) == expectedEntry, "ConfigPatternMap result/config alignment failure");
        if (i < 3)
            require(dut.busy_o && !dut.done_o, "scan stopped early");
        else
            require(!dut.busy_o && dut.done_o, "done was not a final one-cycle pulse");
    }
    tick(dut);
    require(!dut.done_o, "done was not one cycle");
}
} // namespace

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vrecam_role_aware_config_scheduler dut;
    try {
        reset(dut);

        scanAndCheck(dut, false, kAd, true);
        const std::uint32_t adCfg0 = entry(dut, 0);
        require(((adCfg0 >> 10U) & 0x3ffU) != 0 &&
                    (((adCfg0 >> 10U) & 0x3ffU) & (((adCfg0 >> 10U) & 0x3ffU) - 1U)),
                "candidate bitmap was not preserved as a multi-candidate bitmap");
        std::cout << "A/D SEQUENCE PASS\n";
        std::cout << "RESULT ALIGNMENT PASS\n";
        std::cout << "CANDIDATE BITMAP PRESERVATION PASS\n";
        std::cout << "BUSY START NEGATIVE PASS\n";

        bool mixedFound = false;
        for (unsigned seed = 1; seed <= 64 && !mixedFound; ++seed) {
            driveMixedSearchState(dut, seed);
            scanAndCheck(dut, false, kAd, false);
            bool repairable = false;
            bool unrepairable = false;
            for (unsigned i = 0; i < 4; ++i) {
                repairable |= ((entry(dut, i) >> 5U) & 0x1U) != 0;
                unrepairable |= ((entry(dut, i) >> 5U) & 0x1U) == 0;
            }
            mixedFound = repairable && unrepairable;
        }
        require(mixedFound, "could not construct a stable mixed-repairability scan");
        driveStableState(dut);
        std::cout << "MIXED REPAIRABILITY CAPTURE PASS\n";

        scanAndCheck(dut, true, kBc, false);
        for (unsigned i = 0; i < 4; ++i)
            require(((entry(dut, i) >> 1U) & 0x7U) == kBc[i],
                    "restart leaked prior A/D ConfigPatternMap contents");
        std::cout << "B/C SEQUENCE PASS\n";
        std::cout << "RESTART PASS\n";
        std::cout << "PHASE3C_FUNCTIONAL_REGRESSION PASS\n";
    } catch (const std::exception &error) {
        std::cerr << "PHASE3C_FUNCTIONAL_REGRESSION FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
