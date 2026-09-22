#include "Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace {
struct Candidate { bool valid = false; unsigned pattern = 0; };
using Schedule = std::array<std::array<Candidate, 4>, 4>;

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void tick(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void reset(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core &dut)
{
    dut.clk_i = 0;
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.candidate_valid_i = 0;
    dut.candidate_pattern_id_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

struct Result {
    unsigned commits = 0;
    unsigned tokens = 0;
    unsigned configs = 0;
    unsigned patterns = 0;
    unsigned borrows = 0;
    unsigned failure = 0;
    bool repairable = false;
    std::array<int, 4> commitCycle{{-1, -1, -1, -1}};
    int doneCycle = -1;
};

Result run(const Schedule &schedule)
{
    Vrecam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core dut;
    reset(dut);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    Result result;
    unsigned oldCommits = 0;
    for (int cycle = 1; cycle <= 20; ++cycle) {
        const unsigned sa = dut.current_sa_o;
        const unsigned slot = dut.current_slot_o;
        require(sa < 4 && slot < 4, "controller SA/slot in range");
        dut.candidate_valid_i = schedule[sa][slot].valid;
        dut.candidate_pattern_id_i = schedule[sa][slot].pattern;
        tick(dut);
        const unsigned newCommits = dut.sa_commit_valid_o & ~oldCommits;
        if (newCommits != 0) {
            require((newCommits & (newCommits - 1)) == 0,
                    "exactly one immediate SA commit per edge");
            for (unsigned index = 0; index != 4; ++index)
                if ((newCommits & (1U << index)) != 0)
                    result.commitCycle[index] = cycle;
            oldCommits = dut.sa_commit_valid_o;
        }
        if (dut.done_o) {
            result.doneCycle = cycle;
            break;
        }
    }
    require(result.doneCycle >= 0, "controller terminates");
    result.commits = dut.sa_commit_valid_o;
    result.tokens = dut.common_spare_available_o;
    result.configs = dut.selected_config_flat_o;
    result.patterns = dut.selected_pattern_flat_o;
    result.borrows = dut.borrow_flat_o;
    result.failure = dut.failure_position_o;
    result.repairable = dut.group_repairable_o;
    return result;
}

Schedule withSlots(const std::array<unsigned, 4> &slots)
{
    Schedule schedule{};
    for (unsigned sa = 0; sa != 4; ++sa)
        schedule[sa][slots[sa]] = {true, sa + 1};
    return schedule;
}

constexpr std::array<std::array<unsigned, 4>, 4> kMask{{
    {{1, 0, 5, 4}}, {{4, 0, 6, 2}},
    {{8, 0, 9, 1}}, {{2, 0, 10, 8}}
}};

void verifyAllSixteenStaticActions()
{
    for (unsigned sa = 0; sa != 4; ++sa) {
        for (unsigned slot = 0; slot != 4; ++slot) {
            std::array<unsigned, 4> selection{{1, 1, 1, 1}};
            selection[sa] = slot;
            const Result result = run(withSlots(selection));
            if (slot == 2 && sa != 0) {
                // A prior R leaves m=1 available; selected SA borrows once.
                require(result.repairable, "RB action is legal with empty borrow history");
            }
            const unsigned expected = 0x0f & ~kMask[sa][slot];
            require((result.tokens & kMask[sa][slot]) == 0,
                    "selected static action consumes its exact claim mask");
            require((result.tokens | expected) == expected,
                    "token state contains no unlisted claim bit");
        }
    }
}
} // namespace

int main()
{
    verifyAllSixteenStaticActions();

    const Result allR = run(withSlots({{1, 1, 1, 1}}));
    require(allR.repairable && allR.commits == 0x0f,
            "R action succeeds for all SAs");
    require(allR.configs == 0x84c && allR.patterns == 0x4321,
            "R config and PatternIDs commit at their SA edge");
    require(allR.commitCycle == std::array<int, 4>{{1, 2, 3, 4}} &&
                allR.doneCycle == 4,
            "immediate streaming commit timing");

    const Result firstFailure = run(Schedule{});
    require(!firstFailure.repairable && firstFailure.commits == 0 &&
                firstFailure.failure == 0 && firstFailure.doneCycle == 4,
            "first failure terminates after R/L/RB/B without rollback");

    const Result prefixFailure = run(withSlots({{1, 0, 2, 1}}));
    require(prefixFailure.repairable && prefixFailure.commits == 0x0f,
            "directed borrow path succeeds");

    std::cout << "FINAL_EARLY_C2_STATIC_ACTIONS=PASS\n";
    std::cout << "FINAL_EARLY_C2_IMMEDIATE_COMMIT=PASS\n";
    std::cout << "FINAL_EARLY_C2_FIRST_FAILURE=PASS\n";
    return 0;
}
