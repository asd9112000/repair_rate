#include "Vrecam_dss_v2_retained_overlap_core.h"

#include "BistOverlapTimingModel.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>

static void tick(Vrecam_dss_v2_retained_overlap_core& dut) {
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

static void clear_inputs(Vrecam_dss_v2_retained_overlap_core& dut) {
    dut.start_i = 0;
    dut.fault_valid_i = 0;
    dut.fault_sa_i = 0;
    dut.fault_row_i = 0;
    dut.fault_col_i = 0;
    dut.test_done_valid_i = 0;
    dut.test_done_sa_i = 0;
}

static void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

static void send_fault(Vrecam_dss_v2_retained_overlap_core& dut,
                       unsigned sa, unsigned row, unsigned col) {
    require(dut.fault_ready_o, "retained overlap bank unexpectedly not ready");
    dut.fault_valid_i = 1;
    dut.fault_sa_i = sa;
    dut.fault_row_i = row;
    dut.fault_col_i = col;
    tick(dut);
    require(dut.retained_update_event_o & (1U << sa),
            "fault did not produce the retained-update acceptance event");
    clear_inputs(dut);
}

static unsigned generation_of(const Vrecam_dss_v2_retained_overlap_core& dut,
                              unsigned sa) {
    return (dut.fault_generation_flat_o >> (sa * 4U)) & 0xfU;
}

static unsigned analysis_generation_of(const Vrecam_dss_v2_retained_overlap_core& dut,
                                       unsigned sa) {
    return (dut.analysis_generation_flat_o >> (sa * 4U)) & 0xfU;
}

static void reset_and_start(Vrecam_dss_v2_retained_overlap_core& dut) {
    clear_inputs(dut);
    dut.rst_ni = 0;
    tick(dut);
    dut.rst_ni = 1;
    dut.start_i = 1;
    tick(dut);
    clear_inputs(dut);
    require(dut.busy_o, "retained overlap controller did not start");
}

// A current-owner update clears the in-progress four-role sweep.  This helper
// checks the externally observable contract: the fourth uninterrupted sweep
// edge, and not an earlier one, emits candidate-ready for that same generation.
static void require_owner_catchup_in_four_cycles(
    Vrecam_dss_v2_retained_overlap_core& dut, unsigned sa, unsigned generation) {
    for (unsigned elapsed = 1; elapsed <= 4; ++elapsed) {
        tick(dut);
        const bool candidate_ready = dut.latest_candidate_ready_o & (1U << sa);
        if (elapsed < 4)
            require(!candidate_ready, "candidate-ready occurred before four clean sweep edges");
        else {
            require(candidate_ready, "candidate-ready missing on fourth clean sweep edge");
            require(dut.analysis_valid_o & (1U << sa),
                    "candidate-ready did not coincide with analysis-valid");
            require(generation_of(dut, sa) == generation &&
                        analysis_generation_of(dut, sa) == generation,
                    "candidate-ready did not match the latest accepted generation");
        }
    }
}

static void verify_candidate_catchup_timing(Vrecam_dss_v2_retained_overlap_core& dut) {
    // Zero-fault generation 0 still reaches analysis-ready after the initial
    // owner's four role slots; this is not a repair commit assertion.
    reset_and_start(dut);
    require_owner_catchup_in_four_cycles(dut, 0, 0);

    // D1: first pivot.  D2: a fault after an already-ready prior state.
    reset_and_start(dut);
    send_fault(dut, 0, 10, 10);
    require_owner_catchup_in_four_cycles(dut, 0, 1);
    send_fault(dut, 0, 10, 11);
    require_owner_catchup_in_four_cycles(dut, 0, 2);

    // D3: an additional independent pivot.  D4: a same-row Hybrid relation.
    reset_and_start(dut);
    send_fault(dut, 0, 10, 10);
    send_fault(dut, 0, 20, 20);
    require_owner_catchup_in_four_cycles(dut, 0, 2);
    reset_and_start(dut);
    send_fault(dut, 0, 30, 30);
    send_fault(dut, 0, 30, 31);
    require_owner_catchup_in_four_cycles(dut, 0, 2);

    // D5: a row-Must threshold witness (four faults in one row).
    reset_and_start(dut);
    for (unsigned col = 40; col < 44; ++col)
        send_fault(dut, 0, 40, col);
    require_owner_catchup_in_four_cycles(dut, 0, 4);

    // D6: an update immediately after candidate-ready starts a fresh four-edge sweep.
    reset_and_start(dut);
    send_fault(dut, 0, 50, 50);
    require_owner_catchup_in_four_cycles(dut, 0, 1);
    send_fault(dut, 0, 51, 51);
    require_owner_catchup_in_four_cycles(dut, 0, 2);

    // D7: a new fault on the would-be fourth completion edge suppresses stale
    // completion; the last accepted generation alone becomes ready four edges later.
    reset_and_start(dut);
    send_fault(dut, 0, 60, 60);
    for (unsigned elapsed = 1; elapsed <= 3; ++elapsed) {
        tick(dut);
        require(!(dut.latest_candidate_ready_o & 1U),
                "candidate-ready occurred before the fourth sweep edge");
    }
    dut.fault_valid_i = 1;
    dut.fault_sa_i = 0;
    dut.fault_row_i = 60;
    dut.fault_col_i = 61;
    tick(dut);
    require(dut.retained_update_event_o & 1U, "same-edge fault was not accepted");
    require(!(dut.latest_candidate_ready_o & 1U),
            "same-edge fault did not suppress stale candidate-ready");
    clear_inputs(dut);
    require_owner_catchup_in_four_cycles(dut, 0, 2);

    // D8: one global input can accept one fault every clock; four consecutive
    // owner updates defer readiness until four subsequent clean sweep edges.
    reset_and_start(dut);
    for (unsigned event = 0; event < 4; ++event) {
        send_fault(dut, 0, 70, 70 + event);
        require(!(dut.latest_candidate_ready_o & 1U),
                "candidate-ready occurred during back-to-back updates");
    }
    require_owner_catchup_in_four_cycles(dut, 0, 4);

    // D9: known 9-entry post-Must witness for ConfigID 2: three pivots,
    // three row relations, and six column relations.  Advance A through its
    // normal final-selection boundary so B owns the analyzer and evaluates
    // its role order {0, 2, 1, 3}, including ConfigID 2.
    reset_and_start(dut);
    send_fault(dut, 0, 5, 5);
    send_fault(dut, 0, 5, 6);
    send_fault(dut, 0, 5, 7);
    const std::array<std::array<unsigned, 2>, 12> nine_post_must{{
        {{0, 0}}, {{10, 10}}, {{20, 20}}, {{0, 1}}, {{10, 11}}, {{20, 21}},
        {{1, 0}}, {{2, 0}}, {{11, 10}}, {{12, 10}}, {{21, 20}}, {{22, 20}}
    }};
    for (const auto& fault : nine_post_must)
        send_fault(dut, 1, fault[0], fault[1]);
    dut.test_done_valid_i = 1;
    dut.test_done_sa_i = 0;
    tick(dut);
    clear_inputs(dut);
    bool b_owns_analyzer = false;
    for (unsigned elapsed = 0; elapsed < 32; ++elapsed) {
        if (dut.analyzer_owner_sa_o == 1U && dut.busy_o) {
            b_owns_analyzer = true;
            break;
        }
        tick(dut);
    }
    require(b_owns_analyzer, "D9 did not advance to B ownership");
    require_owner_catchup_in_four_cycles(dut, 1, 12);

    // D10: more than nine raw memberships collapse after Must (one pivot plus
    // eleven same-row faults).  The collector accepts all twelve, then uses
    // the same control latency irrespective of candidate survivorship.
    reset_and_start(dut);
    for (unsigned col = 0; col < 12; ++col)
        send_fault(dut, 0, 80, col);
    require_owner_catchup_in_four_cycles(dut, 0, 12);
}

// The shared analyzer has one owner, so B/C/D cannot use the owner-local
// four-edge bound until their A-to-B-to-C-to-D ownership turn begins.  This
// test records every ready event in one successful transaction and checks
// generation matching for all four SAs without inventing a fixed global delay.
static void verify_all_sa_scheduled_readiness(Vrecam_dss_v2_retained_overlap_core& dut) {
    reset_and_start(dut);
    std::array<unsigned, 4> last_accept_cycle{{0, 0, 0, 0}};
    std::array<unsigned, 4> candidate_ready_cycle{{0, 0, 0, 0}};
    std::array<unsigned, 4> expected_generation{{3, 1, 1, 1}};
    unsigned cycle = 0;

    const auto observe = [&]() {
        for (unsigned sa = 0; sa < 4; ++sa) {
            if (dut.latest_candidate_ready_o & (1U << sa)) {
                require(candidate_ready_cycle[sa] == 0,
                        "SA emitted more than one ready edge without a new update");
                candidate_ready_cycle[sa] = cycle;
                require(dut.analysis_valid_o & (1U << sa),
                        "scheduled candidate-ready lacked analysis-valid");
                require(generation_of(dut, sa) == expected_generation[sa] &&
                            analysis_generation_of(dut, sa) == expected_generation[sa],
                        "scheduled candidate-ready used a stale generation");
            }
        }
    };
    const auto drive_fault = [&](unsigned sa, unsigned row, unsigned col) {
        require(dut.fault_ready_o, "scheduled readiness fault was not accepted");
        dut.fault_valid_i = 1;
        dut.fault_sa_i = sa;
        dut.fault_row_i = row;
        dut.fault_col_i = col;
        tick(dut);
        ++cycle;
        require(dut.retained_update_event_o & (1U << sa),
                "scheduled readiness lacked retained-update event");
        last_accept_cycle[sa] = cycle;
        observe();
        clear_inputs(dut);
    };
    const auto drive_done = [&](unsigned sa) {
        dut.test_done_valid_i = 1;
        dut.test_done_sa_i = sa;
        tick(dut);
        ++cycle;
        observe();
        clear_inputs(dut);
    };

    drive_fault(0, 10, 10);
    drive_fault(0, 10, 11);
    drive_fault(0, 10, 12);
    drive_fault(1, 20, 20);
    drive_fault(2, 30, 30);
    drive_fault(3, 40, 40);
    for (unsigned sa = 0; sa < 4; ++sa)
        drive_done(sa);

    for (unsigned elapsed = 0; elapsed < 160 && !dut.done_o; ++elapsed) {
        tick(dut);
        ++cycle;
        observe();
    }
    require(dut.done_o && dut.group_repairable_o,
            "scheduled readiness transaction did not complete successfully");
    for (unsigned sa = 0; sa < 4; ++sa) {
        require(candidate_ready_cycle[sa] != 0,
                "scheduled readiness missed an SA candidate-ready event");
        require(candidate_ready_cycle[sa] - last_accept_cycle[sa] >= 4,
                "scheduled candidate-ready violated the four clean-edge lower bound");
    }
}

// This is the direct T1 C++-model versus RTL-harness comparison.  It uses a
// successful retained-controller trace so the model's timing-only virtual
// ownership release corresponds to the RTL's observed A-to-B-to-C-to-D path.
static void verify_cpp_model_against_rtl(Vrecam_dss_v2_retained_overlap_core& dut) {
    dynamic_spare::BistOverlapTimingInput input;
    input.testStartCycles = {{0, 0, 0, 0}};
    input.testDoneCycles = {{7, 8, 9, 10}};
    input.faultEvents = {{1, 0, 1}, {2, 0, 1}, {3, 0, 1},
                         {4, 1, 1}, {5, 2, 1}, {6, 3, 1}};
    dynamic_spare::BistOverlapTimingModel model;
    const auto expected = model.evaluateEventStream(input);

    reset_and_start(dut);
    std::array<unsigned, 4> rtl_ready_cycle{{0, 0, 0, 0}};
    for (unsigned cycle = 1; cycle <= 32; ++cycle) {
        clear_inputs(dut);
        switch (cycle) {
            case 1: dut.fault_valid_i = 1; dut.fault_sa_i = 0;
                    dut.fault_row_i = 10; dut.fault_col_i = 10; break;
            case 2: dut.fault_valid_i = 1; dut.fault_sa_i = 0;
                    dut.fault_row_i = 10; dut.fault_col_i = 11; break;
            case 3: dut.fault_valid_i = 1; dut.fault_sa_i = 0;
                    dut.fault_row_i = 10; dut.fault_col_i = 12; break;
            case 4: dut.fault_valid_i = 1; dut.fault_sa_i = 1;
                    dut.fault_row_i = 20; dut.fault_col_i = 20; break;
            case 5: dut.fault_valid_i = 1; dut.fault_sa_i = 2;
                    dut.fault_row_i = 30; dut.fault_col_i = 30; break;
            case 6: dut.fault_valid_i = 1; dut.fault_sa_i = 3;
                    dut.fault_row_i = 40; dut.fault_col_i = 40; break;
            default: break;
        }
        if (cycle >= 7 && cycle <= 10) {
            dut.test_done_valid_i = 1;
            dut.test_done_sa_i = cycle - 7;
        }
        if (dut.fault_valid_i)
            require(dut.fault_ready_o, "RTL comparison fault was not accepted");
        tick(dut);
        for (unsigned sa = 0; sa < 4; ++sa) {
            if (dut.latest_candidate_ready_o & (1U << sa)) {
                require(rtl_ready_cycle[sa] == 0,
                        "RTL comparison emitted duplicate candidate-ready edge");
                rtl_ready_cycle[sa] = cycle;
                require(dut.analysis_valid_o & (1U << sa),
                        "RTL comparison candidate-ready lacked analysis-valid");
            }
        }
    }
    for (unsigned sa = 0; sa < 4; ++sa) {
        require(rtl_ready_cycle[sa] == expected.subarrays[sa].candidateReadyCycle,
                "C++ timing model and RTL candidate-ready cycle differ");
        require(((dut.analysis_generation_flat_o >> (sa * 4U)) & 0xfU) ==
                    expected.subarrays[sa].latestGeneration,
                "C++ timing model and RTL final generation differ");
    }
}

static void randomized_generation_stress(Vrecam_dss_v2_retained_overlap_core& dut)
{
    std::mt19937 random(20260915U);
    for (unsigned vector = 0; vector < 1000; ++vector) {
        clear_inputs(dut);
        dut.rst_ni = 0;
        tick(dut);
        dut.rst_ni = 1;
        dut.start_i = 1;
        tick(dut);
        clear_inputs(dut);
        std::array<unsigned, 4> expected_generation{{0, 0, 0, 0}};
        std::array<bool, 4> committed{{false, false, false, false}};
        unsigned prior_commits = 0;

        // These updates arrive at varied owner/sweep positions.  The model
        // records only accepted updates, then requires any visible commit to
        // be for that exact latest generation.
        for (unsigned event = 0; event < 12; ++event) {
            const unsigned sa = random() & 3U;
            dut.fault_valid_i = 1;
            dut.fault_sa_i = sa;
            dut.fault_row_i = 100 + ((random() + event) & 31U);
            dut.fault_col_i = 200 + ((random() + event * 3U) & 31U);
            tick(dut);
            if (dut.retained_update_event_o & (1U << sa))
                ++expected_generation[sa];
            clear_inputs(dut);
            for (unsigned delay = 0; delay < (random() % 3); ++delay)
                tick(dut);
        }
        std::array<unsigned, 4> done_order{{0, 1, 2, 3}};
        for (unsigned index = 0; index < 4; ++index) {
            const unsigned swap_index = index + (random() % (4 - index));
            const unsigned temporary = done_order[index];
            done_order[index] = done_order[swap_index];
            done_order[swap_index] = temporary;
        }
        for (unsigned sa : done_order) {
            dut.test_done_valid_i = 1;
            dut.test_done_sa_i = sa;
            tick(dut);
            clear_inputs(dut);
            for (unsigned delay = 0; delay < (random() % 3); ++delay)
                tick(dut);
        }
        bool terminal = !dut.busy_o;
        for (unsigned cycle = 0; cycle < 200; ++cycle) {
            tick(dut);
            const unsigned changed = dut.sa_commit_valid_o & ~prior_commits;
            for (unsigned sa = 0; sa < 4; ++sa) {
                if (changed & (1U << sa)) {
                    const unsigned fault_generation =
                        (dut.fault_generation_flat_o >> (sa * 4U)) & 0xfU;
                    const unsigned analysis_generation =
                        (dut.analysis_generation_flat_o >> (sa * 4U)) & 0xfU;
                    require(fault_generation == expected_generation[sa] &&
                                analysis_generation == expected_generation[sa],
                            "stale generation committed in randomized stress");
                    committed[sa] = true;
                }
            }
            prior_commits = dut.sa_commit_valid_o;
            if (dut.done_o || !dut.busy_o) {
                terminal = true;
                break;
            }
        }
        require(terminal, "randomized retained overlap transaction timed out");
        (void)committed;
    }
}

int main() {
    try {
        Vrecam_dss_v2_retained_overlap_core dut;
        verify_candidate_catchup_timing(dut);
        verify_all_sa_scheduled_readiness(dut);
        verify_cpp_model_against_rtl(dut);

        // Start once, then accumulate three generations for A while the
        // shared analyzer is active.  The controller must discard its old
        // role sweep and later commit only the third generation.
        reset_and_start(dut);
        send_fault(dut, 0, 10, 10);  // generation 1: pivot
        send_fault(dut, 0, 10, 11);  // generation 2: Hybrid/reuse relation
        send_fault(dut, 0, 10, 12);  // generation 3: latest state
        require((dut.fault_generation_flat_o & 0xfU) == 3,
                "A fault generation did not reach generation 3");

        // Later-SA collection is independent of A's analyzer ownership.
        send_fault(dut, 1, 20, 20);
        send_fault(dut, 2, 30, 30);
        send_fault(dut, 3, 40, 40);

        for (unsigned sa = 0; sa < 4; ++sa) {
            dut.test_done_valid_i = 1;
            dut.test_done_sa_i = sa;
            tick(dut);
            clear_inputs(dut);
        }

        std::array<unsigned, 4> commit_cycle{{0, 0, 0, 0}};
        unsigned previous_commits = 0;
        bool done = false;
        for (unsigned cycle = 1; cycle <= 160; ++cycle) {
            tick(dut);
            const unsigned changed = dut.sa_commit_valid_o & ~previous_commits;
            for (unsigned sa = 0; sa < 4; ++sa) {
                if (changed & (1U << sa))
                    commit_cycle[sa] = cycle;
            }
            previous_commits = dut.sa_commit_valid_o;
            if (dut.done_o) {
                done = true;
                break;
            }
        }
        require(done, "retained overlap controller did not reach terminal state");
        require(dut.group_repairable_o, "directed retained overlap case was not repairable");
        require(dut.sa_commit_valid_o == 0xf, "not all four SAs committed");
        require(commit_cycle[0] && commit_cycle[1] && commit_cycle[2] && commit_cycle[3],
                "missing commit edge");
        require(commit_cycle[0] < commit_cycle[1] && commit_cycle[1] < commit_cycle[2] &&
                    commit_cycle[2] < commit_cycle[3],
                "A-to-B-to-C-to-D ownership/commit order changed");
        require((dut.analysis_generation_flat_o & 0xfU) == 3,
                "A final analysis generation was not generation 3");
        randomized_generation_stress(dut);
        std::cout << "S1GA2G_RETAINED_OVERLAP_CORE_TEST PASS\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "S1GA2G_RETAINED_OVERLAP_CORE_TEST FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
