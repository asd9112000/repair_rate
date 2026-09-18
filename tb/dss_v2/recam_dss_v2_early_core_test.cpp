#include "Vrecam_dss_v2_early_core.h"

#include <array>
#include <cstdlib>
#include <iomanip>
#include <iostream>

struct Candidate {
    bool local_accept = false;
    unsigned pattern_id = 0;
};
using Schedule = std::array<std::array<Candidate, 7>, 4>;

struct Result {
    unsigned commits = 0;
    unsigned ledger = 0;
    unsigned released = 0;
    unsigned borrowed = 0;
    unsigned borrower_ids = 0;
    unsigned configs = 0;
    unsigned patterns = 0;
    unsigned donors = 0;
    unsigned borrow_actions = 0;
    unsigned release_actions = 0;
    unsigned failure_position = 0;
    bool repairable = false;
    std::array<unsigned, 4> ledger_after_commit{};
    unsigned commit_count = 0;
    // Cycle zero is the rising edge accepting start_i. Each stored value is
    // an assertion edge after that start edge; no DUT state is added.
    std::array<int, 4> commit_cycle{{-1, -1, -1, -1}};
    int done_cycle = -1;
};

static void require(bool condition, const char* what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        std::exit(1);
    }
}

static void tick(Vrecam_dss_v2_early_core& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

static void initialise(Vrecam_dss_v2_early_core& dut) {
    dut.clk_i = 0;
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.candidate_solution_valid_i = 0;
    dut.candidate_repairable_i = 0;
    dut.candidate_pattern_id_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

static void select(Schedule& schedule, unsigned sa, unsigned config,
                   unsigned pattern_id) {
    schedule.at(sa).at(config) = {true, pattern_id};
}

// The harness supplies only the current analyzer result.  The core itself
// performs ranking, Phase 4B feasibility, Phase 4C commit, and traversal.
static Result run(const Schedule& schedule) {
    Vrecam_dss_v2_early_core dut;
    initialise(dut);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;

    Result result;
    unsigned previous_commits = 0;
    for (unsigned cycle = 1; cycle <= 40; ++cycle) {
        const unsigned sa = dut.current_sa_o;
        const unsigned config = dut.current_config_id_o;
        require(sa < 4 && config < 7, "core current candidate is in range");
        const Candidate& candidate = schedule[sa][config];
        dut.candidate_solution_valid_i = candidate.local_accept;
        dut.candidate_repairable_i = candidate.local_accept;
        dut.candidate_pattern_id_i = candidate.pattern_id;
        dut.eval();
        tick(dut);

        if (dut.sa_commit_valid_o != previous_commits) {
            require((dut.sa_commit_valid_o & previous_commits) == previous_commits,
                    "commit bitmap is monotonic");
            require(result.commit_count < 4, "at most one commit per SA");
            const unsigned new_commits =
                dut.sa_commit_valid_o & ~previous_commits;
            require(new_commits != 0 && (new_commits & (new_commits - 1)) == 0,
                    "each decision edge commits exactly one SA");
            for (unsigned sa = 0; sa != 4; ++sa) {
                if ((new_commits & (1U << sa)) != 0)
                    result.commit_cycle[sa] = static_cast<int>(cycle);
            }
            result.ledger_after_commit[result.commit_count++] =
                dut.ledger_released_borrower_o;
            previous_commits = dut.sa_commit_valid_o;
        }
        if (dut.done_o) {
            result.done_cycle = static_cast<int>(cycle);
            break;
        }
    }
    require(dut.done_o, "core terminates deterministically");
    result.commits = dut.sa_commit_valid_o;
    result.ledger = dut.ledger_released_borrower_o;
    result.released = dut.canonical_released_o;
    result.borrowed = dut.canonical_borrowed_o;
    result.borrower_ids = dut.canonical_borrower_id_flat_o;
    result.configs = dut.selected_config_flat_o;
    result.patterns = dut.selected_pattern_flat_o;
    result.donors = dut.selected_donor_flat_o;
    result.borrow_actions = dut.borrow_flat_o;
    result.release_actions = dut.release_flat_o;
    result.failure_position = dut.failure_position_o;
    result.repairable = dut.group_repairable_o;
    return result;
}

static void printDecisionTiming(const char* name, const Result& result) {
    unsigned committed = 0;
    unsigned total = 0;
    for (const int cycle : result.commit_cycle) {
        if (cycle >= 0) {
            ++committed;
            total += static_cast<unsigned>(cycle);
        }
    }
    std::cout << "S1B1_DECISION_TIMING policy=EARLY case=" << name
              << " start=0 A=" << result.commit_cycle[0]
              << " B=" << result.commit_cycle[1]
              << " C=" << result.commit_cycle[2]
              << " D=" << result.commit_cycle[3]
              << " done=" << result.done_cycle
              << " group_cycles=" << result.done_cycle
              << " average_committed_sa_cycles=";
    if (committed == 0)
        std::cout << "N/A";
    else
        std::cout << std::fixed << std::setprecision(2)
                  << static_cast<double>(total) / committed;
    std::cout << " terminal="
              << (result.repairable ? "SUCCESS" : "FAILURE")
              << " failure_position=" << result.failure_position << '\n';
}

static Schedule all_local() {
    Schedule schedule{};
    for (unsigned sa = 0; sa != 4; ++sa)
        select(schedule, sa, 0, sa + 1);
    return schedule;
}

int main() {
    {
        const Result result = run(all_local());
        require(result.repairable && result.commits == 0xf, "full local group succeeds");
        require(result.ledger == 0 && result.released == 0 && result.borrowed == 0,
                "full local group leaves ledger empty");
        require(result.configs == 0 && result.patterns == 0x4321,
                "full local selected ConfigID and PatternID trace");
        require(result.commit_cycle == std::array<int, 4>{{1, 2, 3, 4}} &&
                    result.done_cycle == 4,
                "EARLY all-local decision timing contract");
        printDecisionTiming("ALL_LOCAL", result);
        std::cout << "V2_EARLY_FULL_LOCAL PASS\n";
    }

    {
        Schedule schedule{};
        select(schedule, 0, 4, 9); // A releases A_ROW.
        select(schedule, 1, 2, 7); // B borrows A_ROW.
        select(schedule, 2, 0, 3);
        select(schedule, 3, 0, 4);
        const Result result = run(schedule);
        require(result.repairable && result.commits == 0xf, "release then borrow group succeeds");
        require(result.configs == 0x14 && result.patterns == 0x4379,
                "selected ConfigID and PatternID propagate per SA");
        require(result.release_actions == 0x1 && result.borrow_actions == 0x2,
                "only selected A release and B borrow actions are committed");
        require(((result.donors >> 2) & 3) == 0, "B uses A_ROW primary donor");
        require(result.released == 0x1 && result.borrowed == 0x1 &&
                    (result.borrower_ids & 3) == 1 && result.ledger == 0x011,
                "canonical and legacy ledger show A_ROW borrowed by B");
        require(result.ledger_after_commit[0] == 0x001 &&
                    result.ledger_after_commit[1] == 0x011,
                "latest committed ledger feeds the next SA");
        printDecisionTiming("RELEASE_THEN_BORROW", result);
        std::cout << "LATEST_LEDGER_PROPAGATION PASS\n";
    }

    {
        Schedule schedule{};
        select(schedule, 0, 4, 1);
        select(schedule, 1, 2, 2); // consumes A_ROW.
        select(schedule, 2, 2, 3); // C can use neither D_ROW nor consumed A_ROW.
        const Result result = run(schedule);
        require(!result.repairable && result.commits == 0x3 && result.failure_position == 2,
                "consumed resource is not allocated twice");
        require(result.ledger == 0x011 && result.commit_count == 2,
                "failed candidate cannot commit or roll back state");
        printDecisionTiming("RESOURCE_CONSUMPTION_FAILURE_C", result);
        std::cout << "RESOURCE_CONSUMPTION PASS\n";
    }

    for (unsigned failure_sa = 0; failure_sa != 4; ++failure_sa) {
        Schedule schedule{};
        for (unsigned sa = 0; sa != failure_sa; ++sa) {
            if (sa == 0)
                select(schedule, sa, 4, 5); // retain a visible no-rollback state.
            else
                select(schedule, sa, 0, sa + 1);
        }
        const Result result = run(schedule);
        const unsigned expected_commits = (1u << failure_sa) - 1u;
        require(!result.repairable && result.commits == expected_commits &&
                    result.failure_position == failure_sa,
                "first failure stops at the expected SA");
        if (failure_sa != 0)
            require(result.ledger == 0x001, "first failure preserves prior committed ledger");
        else
            require(result.ledger == 0, "failure at A has no prior ledger state");
        printDecisionTiming(
            failure_sa == 0 ? "FAILURE_A" :
            failure_sa == 1 ? "FAILURE_B" :
            failure_sa == 2 ? "FAILURE_C" : "FAILURE_D", result);
        std::cout << "FAILURE_AT_" << static_cast<char>('A' + failure_sa) << " PASS\n";
    }

    {
        Schedule schedule = all_local();
        select(schedule, 0, 4, 8); // valid but lower priority than CFG0.
        const Result result = run(schedule);
        require(result.repairable && result.configs == 0 && result.ledger == 0 &&
                    result.commit_count == 4,
                "non-selected acceptable candidate does not commit");
        std::cout << "NON_SELECTED_CANDIDATE_NO_COMMIT PASS\n";
    }

    std::cout << "FIRST_FAILURE_TERMINATION PASS\n";
    std::cout << "NO_ROLLBACK PASS\n";
    std::cout << "PATTERN_ID_PROPAGATION PASS\n";
    return 0;
}
