#include "Vrecam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_atomic_group_commit.h"
#include "verilated.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace
{
using Dut = Vrecam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_atomic_group_commit;

void tick(Dut &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void reset(Dut &dut)
{
    dut.rst_ni = 0;
    dut.start_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

bool run(Dut &dut, std::uint8_t valid, std::uint8_t action, std::uint8_t release,
         std::uint8_t borrow, std::uint8_t donor, bool expect_success)
{
    const std::uint32_t initial_released = dut.resource_released_o;
    const std::uint32_t initial_borrowed = dut.resource_borrowed_o;
    dut.selected_valid_i = valid;
    dut.selected_action_flat_i = action;
    dut.selected_release_i = release;
    dut.selected_borrow_i = borrow;
    dut.selected_donor_flat_i = donor;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    if (dut.commit_accepted_o || dut.commit_error_o)
        return (dut.commit_accepted_o != 0U) == expect_success;
    for (int cycle = 0; cycle < 16; ++cycle)
    {
        if (dut.resource_released_o != initial_released && !dut.commit_accepted_o)
            throw std::runtime_error("persistent release state became visible before publish");
        if (dut.resource_borrowed_o != initial_borrowed && !dut.commit_accepted_o)
            throw std::runtime_error("persistent borrower state became visible before publish");
        tick(dut);
        if (dut.commit_accepted_o || dut.commit_error_o)
            return (dut.commit_accepted_o != 0U) == expect_success;
    }
    throw std::runtime_error("atomic commit timeout");
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Dut dut;
    std::size_t atomicity_violations = 0;
    std::size_t partial_visibility_errors = 0;
    std::size_t corruption_errors = 0;
    try {
        reset(dut);
        if (!run(dut, 0x0f, 0x55, 0x0f, 0x00, 0xc6, true) || dut.resource_released_o != 0x0f)
            ++atomicity_violations;
        if (!run(dut, 0x0f, 0x55, 0x0f, 0x00, 0xc6, false) || dut.resource_released_o != 0x0f)
            ++corruption_errors;
        for (int failure_stage = 0; failure_stage < 4; ++failure_stage) {
            reset(dut);
            const std::uint8_t valid_prefix = static_cast<std::uint8_t>((1U << failure_stage) - 1U);
            if (!run(dut, valid_prefix, 0x55, 0x0f, 0x00, 0xc6, false))
                ++atomicity_violations;
            if (dut.resource_released_o != 0U || dut.resource_borrowed_o != 0U)
                ++corruption_errors;
        }
        reset(dut);
        if (!run(dut, 0x0f, 0x56, 0x01, 0x01, 0xc4, false))
            ++atomicity_violations;
        if (dut.resource_released_o != 0U || dut.resource_borrowed_o != 0U)
            ++partial_visibility_errors;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "ATOMICITY_VIOLATIONS=" << atomicity_violations << '\n'
              << "PARTIAL_COMMIT_VISIBILITY_ERRORS=" << partial_visibility_errors << '\n'
              << "COMMIT_ERROR_STATE_CORRUPTION=" << corruption_errors << '\n';
    return atomicity_violations == 0 && partial_visibility_errors == 0 && corruption_errors == 0 ? 0 : 1;
}
