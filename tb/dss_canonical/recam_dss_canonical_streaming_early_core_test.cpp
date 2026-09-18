#include "Vrecam_dss_canonical_streaming_early_core.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
constexpr unsigned kSaCount = 4;
constexpr unsigned kActionCount = 4;
constexpr std::array<unsigned, kActionCount> kPriority{{1, 0, 3, 2}};

struct Candidate
{
    bool valid = false;
    unsigned pattern = 0;
};

using CandidateMap = std::array<Candidate, kSaCount * kActionCount>;

struct Result
{
    bool repairable = false;
    unsigned failurePosition = 3;
    unsigned commits = 0;
    unsigned actions = 0;
    unsigned configs = 0;
    unsigned patterns = 0;
    unsigned donors = 0;
    unsigned borrows = 0;
    unsigned releases = 0;
    unsigned releasedState = 0;
    unsigned borrowedState = 0;
    unsigned borrowerIds = 0;
    std::vector<std::array<unsigned, 3>> ledgerAfterCommit;
};

unsigned configId(unsigned resourcePoint, unsigned sa, unsigned action)
{
    if (resourcePoint == 2)
    {
        static constexpr unsigned rowRole[4] = {0, 4, 5, 6};
        static constexpr unsigned columnRole[4] = {0, 1, 2, 3};
        return (sa == 0 || sa == 3) ? rowRole[action] : columnRole[action];
    }
    static constexpr unsigned rowRole[4] = {0, 1, 2, 3};
    static constexpr unsigned columnRole[4] = {0, 4, 5, 6};
    return (sa == 0 || sa == 3) ? rowRole[action] : columnRole[action];
}

unsigned releaseResource(unsigned sa)
{
    static constexpr unsigned resource[4] = {0, 2, 3, 1};
    return resource[sa];
}

std::array<unsigned, 2> donors(unsigned sa)
{
    static constexpr std::array<std::array<unsigned, 2>, 4> order{{
        {{2, 3}}, {{0, 1}}, {{1, 0}}, {{3, 2}}
    }};
    return order[sa];
}

Result golden(const CandidateMap &map, unsigned resourcePoint)
{
    Result result;
    unsigned released = 0;
    unsigned borrowed = 0;
    unsigned borrowerIds = 0;

    for (unsigned sa = 0; sa < kSaCount; ++sa)
    {
        bool selected = false;
        for (const unsigned action : kPriority)
        {
            const Candidate &candidate = map[sa * kActionCount + action];
            if (!candidate.valid)
                continue;

            const bool release = action == 1 || action == 3;
            const bool borrow = action == 2 || action == 3;
            unsigned donor = 0;
            bool donorValid = !borrow;
            if (borrow)
            {
                for (const unsigned possible : donors(sa))
                {
                    if ((released & (1U << possible)) != 0 &&
                        (borrowed & (1U << possible)) == 0)
                    {
                        donor = possible;
                        donorValid = true;
                        break;
                    }
                }
            }
            if (!donorValid)
                continue;

            if (release)
                released |= 1U << releaseResource(sa);
            if (borrow)
            {
                borrowed |= 1U << donor;
                borrowerIds &= ~(3U << (2 * donor));
                borrowerIds |= sa << (2 * donor);
            }

            result.commits |= 1U << sa;
            result.actions |= action << (2 * sa);
            result.configs |= configId(resourcePoint, sa, action) << (3 * sa);
            result.patterns |= candidate.pattern << (6 * sa);
            result.donors |= donor << (2 * sa);
            result.borrows |= unsigned(borrow) << sa;
            result.releases |= unsigned(release) << sa;
            result.ledgerAfterCommit.push_back({released, borrowed, borrowerIds});
            selected = true;
            break;
        }
        if (!selected)
        {
            result.failurePosition = sa;
            result.releasedState = released;
            result.borrowedState = borrowed;
            result.borrowerIds = borrowerIds;
            return result;
        }
    }

    result.repairable = true;
    result.releasedState = released;
    result.borrowedState = borrowed;
    result.borrowerIds = borrowerIds;
    return result;
}

void tick(Vrecam_dss_canonical_streaming_early_core &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

bool runCase(Vrecam_dss_canonical_streaming_early_core &dut,
             const CandidateMap &map,
             unsigned resourcePoint,
             const std::string &name)
{
    const Result expected = golden(map, resourcePoint);
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.candidate_solution_valid_i = 0;
    dut.candidate_repairable_i = 0;
    dut.candidate_pattern_id_i = 0;
    tick(dut);
    dut.rst_ni = 1;
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;

    unsigned previousCommits = 0;
    std::size_t observedCommitCount = 0;
    unsigned cycles = 0;
    while (!dut.done_o && cycles < 32)
    {
        dut.eval();
        const unsigned sa = dut.current_sa_o;
        const unsigned action = dut.current_action_o;
        const Candidate &candidate = map[sa * kActionCount + action];
        dut.candidate_solution_valid_i = candidate.valid;
        dut.candidate_repairable_i = candidate.valid;
        dut.candidate_pattern_id_i = candidate.pattern;
        tick(dut);
        const unsigned commits = dut.sa_commit_valid_o;
        if (commits != previousCommits)
        {
            if (observedCommitCount >= expected.ledgerAfterCommit.size())
            {
                std::cerr << name << ": unexpected commit\n";
                return false;
            }
            const auto &snapshot = expected.ledgerAfterCommit[observedCommitCount];
            if (dut.resource_released_o != snapshot[0] ||
                dut.resource_borrowed_o != snapshot[1] ||
                dut.borrower_id_flat_o != snapshot[2])
            {
                std::cerr << name << ": post-commit ledger mismatch at commit "
                          << observedCommitCount << '\n';
                return false;
            }
            previousCommits = commits;
            ++observedCommitCount;
        }
        ++cycles;
    }

    const bool pass = dut.done_o &&
        bool(dut.group_repairable_o) == expected.repairable &&
        dut.failure_position_o == expected.failurePosition &&
        dut.sa_commit_valid_o == expected.commits &&
        dut.selected_action_flat_o == expected.actions &&
        dut.selected_config_flat_o == expected.configs &&
        dut.selected_pattern_flat_o == expected.patterns &&
        dut.selected_donor_flat_o == expected.donors &&
        dut.borrow_flat_o == expected.borrows &&
        dut.release_flat_o == expected.releases &&
        dut.resource_released_o == expected.releasedState &&
        dut.resource_borrowed_o == expected.borrowedState &&
        dut.borrower_id_flat_o == expected.borrowerIds &&
        observedCommitCount == expected.ledgerAfterCommit.size();

    if (!pass)
    {
        std::cerr << name << ": mismatch"
                  << " expected repair/fail/commit/action/config/pattern/donor/borrow/release/ledger="
                  << expected.repairable << '/' << expected.failurePosition << '/'
                  << expected.commits << '/' << expected.actions << '/'
                  << expected.configs << '/' << expected.patterns << '/'
                  << expected.donors << '/' << expected.borrows << '/'
                  << expected.releases << '/' << expected.releasedState << ':'
                  << expected.borrowedState << ':' << expected.borrowerIds << '\n'
                  << " observed=" << unsigned(dut.group_repairable_o) << '/'
                  << unsigned(dut.failure_position_o) << '/'
                  << unsigned(dut.sa_commit_valid_o) << '/'
                  << unsigned(dut.selected_action_flat_o) << '/'
                  << unsigned(dut.selected_config_flat_o) << '/'
                  << unsigned(dut.selected_pattern_flat_o) << '/'
                  << unsigned(dut.selected_donor_flat_o) << '/'
                  << unsigned(dut.borrow_flat_o) << '/'
                  << unsigned(dut.release_flat_o) << '/'
                  << unsigned(dut.resource_released_o) << ':'
                  << unsigned(dut.resource_borrowed_o) << ':'
                  << unsigned(dut.borrower_id_flat_o) << '\n';
    }
    return pass;
}

CandidateMap localOnly()
{
    CandidateMap map{};
    for (unsigned sa = 0; sa < kSaCount; ++sa)
        map[sa * kActionCount] = {true, sa + 1};
    return map;
}
} // namespace

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    const unsigned resourcePoint = argc > 1 ? std::stoul(argv[1]) : 2;
    const unsigned randomVectors = argc > 2 ? std::stoul(argv[2]) : 1000;
    Vrecam_dss_canonical_streaming_early_core dut;
    unsigned mismatches = 0;

    CandidateMap e1 = localOnly();
    e1[1] = {true, 9};
    mismatches += !runCase(dut, e1, resourcePoint, "E1_RELEASE_BEFORE_LOCAL");

    CandidateMap e2 = localOnly();
    mismatches += !runCase(dut, e2, resourcePoint, "E2_LOCAL_FALLBACK");

    CandidateMap e3 = localOnly();
    e3[0] = {false, 0};
    e3[1] = {true, 3};
    e3[4] = {false, 0};
    e3[7] = {true, 7};
    e3[6] = {true, 8};
    mismatches += !runCase(dut, e3, resourcePoint, "E3_RB_BEFORE_BORROW");

    CandidateMap e4{};
    e4[1] = {true, 1};
    e4[5] = {true, 2};
    mismatches += !runCase(dut, e4, resourcePoint, "E4_NO_ROLLBACK");

    CandidateMap e5 = localOnly();
    e5[4] = {false, 0};
    e5[5] = {true, 5};
    e5[8] = {false, 0};
    e5[9] = {true, 6};
    e5[12] = {false, 0};
    e5[14] = {true, 10};
    mismatches += !runCase(dut, e5, resourcePoint, "E5_DONOR_PRIORITY");

    // Candidate-map reconstruction of the retained N2/F16/group172
    // canonical greedy-failure witness.  A commits LOCAL, B commits
    // RELEASE_ONLY, and C's sole BORROW_ONLY candidate cannot use A_ROW
    // because A did not release it.  The committed A/B prefix is retained.
    CandidateMap witness{};
    witness[0] = {true, 3};
    witness[2] = {true, 3};
    witness[3] = {true, 2};
    witness[4] = {true, 1};
    witness[5] = {true, 1};
    witness[6] = {true, 1};
    witness[7] = {true, 1};
    witness[10] = {true, 3};
    mismatches += !runCase(dut, witness, resourcePoint,
                           "W1_N2_F16_GROUP172_PREFIX_FAILURE");

    std::mt19937 rng(0x20260918U + resourcePoint);
    for (unsigned vector = 0; vector < randomVectors; ++vector)
    {
        CandidateMap map{};
        for (unsigned entry = 0; entry < map.size(); ++entry)
        {
            map[entry].valid = (rng() % 100) < 60;
            map[entry].pattern = 1 + (rng() % (resourcePoint == 2 ? 10 : 35));
        }
        mismatches += !runCase(dut, map, resourcePoint,
                               "RANDOM_" + std::to_string(vector));
    }

    std::cout << "NORMALIZED_STREAMING_EARLY_RESOURCE_POINT=" << resourcePoint
              << " DIRECTED_VECTORS=6 RANDOM_VECTORS=" << randomVectors
              << " MISMATCHES=" << mismatches << '\n';
    dut.final();
    return mismatches == 0 ? 0 : 1;
}
