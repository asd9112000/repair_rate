#include "Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

constexpr std::array<unsigned, 4> kPriority{{1, 0, 3, 2}};

struct Candidate { bool valid = false; unsigned pattern = 0; };
using Schedule = std::array<std::array<Candidate, 4>, 4>;

struct OracleResult {
    bool repairable = false;
    std::array<int, 4> slots{{-1, -1, -1, -1}};
    std::array<unsigned, 4> configs{{0, 0, 0, 0}};
    std::array<unsigned, 4> patterns{{0, 0, 0, 0}};
    unsigned failure = 0;
};

struct DutResult {
    bool repairable = false;
    unsigned commits = 0;
    unsigned configs = 0;
    unsigned patterns = 0;
    unsigned failure = 0;
};

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

bool releases(unsigned slot) { return slot == 1 || slot == 3; }
bool borrows(unsigned slot) { return slot == 2 || slot == 3; }

bool legalPath(const std::array<unsigned, 4> &slots)
{
    return (!borrows(slots[2]) || releases(slots[0])) &&
           (!borrows(slots[1]) || releases(slots[3])) &&
           (!borrows(slots[0]) || releases(slots[1])) &&
           (!borrows(slots[3]) || releases(slots[2]));
}

bool prefixCompatible(const std::array<int, 4> &selected,
                      unsigned currentSa, unsigned candidateSlot)
{
    for (unsigned tuple = 0; tuple < 256; ++tuple) {
        const std::array<unsigned, 4> slots{{
            tuple & 3U, (tuple >> 2U) & 3U,
            (tuple >> 4U) & 3U, (tuple >> 6U) & 3U}};
        if (!legalPath(slots))
            continue;
        bool matchesPrefix = slots[currentSa] == candidateSlot;
        for (unsigned sa = 0; sa < currentSa; ++sa)
            matchesPrefix = matchesPrefix &&
                slots[sa] == static_cast<unsigned>(selected[sa]);
        if (matchesPrefix)
            return true;
    }
    return false;
}

unsigned configId(unsigned sa, unsigned slot)
{
    if (sa == 1 || sa == 2)
        return slot;
    switch (slot) {
        case 0: return 0;
        case 1: return 4;
        case 2: return 5;
        default: return 6;
    }
}

OracleResult oracle(const Schedule &schedule)
{
    OracleResult result;
    for (unsigned sa = 0; sa < 4; ++sa) {
        for (const unsigned slot : kPriority) {
            if (!schedule[sa][slot].valid ||
                !prefixCompatible(result.slots, sa, slot))
                continue;
            result.slots[sa] = static_cast<int>(slot);
            result.configs[sa] = configId(sa, slot);
            result.patterns[sa] = schedule[sa][slot].pattern;
            break;
        }
        if (result.slots[sa] < 0) {
            result.failure = sa;
            return result;
        }
    }
    result.repairable = true;
    return result;
}

void tick(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
}

void reset(Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core &dut)
{
    dut.clk_i = 0;
    dut.rst_ni = 0;
    dut.start_i = 0;
    dut.candidate_valid_i = 0;
    dut.candidate_pattern_id_i = 0;
    tick(dut);
    dut.rst_ni = 1;
}

DutResult run(const Schedule &schedule)
{
    Vrecam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core dut;
    reset(dut);
    dut.start_i = 1;
    tick(dut);
    dut.start_i = 0;
    for (unsigned cycle = 0; cycle < 16 && !dut.done_o; ++cycle) {
        const unsigned sa = dut.current_sa_o;
        const unsigned slot = dut.current_slot_o;
        require(sa < 4 && slot < 4, "controller source slot is in range");
        dut.candidate_valid_i = schedule[sa][slot].valid;
        dut.candidate_pattern_id_i = schedule[sa][slot].pattern;
        tick(dut);
    }
    require(dut.done_o != 0, "controller completes within four ranks per SA");
    return {dut.group_repairable_o != 0, dut.sa_commit_valid_o,
            dut.selected_config_flat_o, dut.selected_pattern_flat_o,
            dut.failure_position_o};
}

void compare(const Schedule &schedule, unsigned &mismatches)
{
    const OracleResult expected = oracle(schedule);
    const DutResult actual = run(schedule);
    unsigned expectedCommitMask = 0;
    unsigned expectedConfigs = 0;
    unsigned expectedPatterns = 0;
    for (unsigned sa = 0; sa < 4; ++sa) {
        if (expected.slots[sa] < 0)
            break;
        expectedCommitMask |= 1U << sa;
        expectedConfigs |= expected.configs[sa] << (3U * sa);
        expectedPatterns |= expected.patterns[sa] << (4U * sa);
    }
    if (actual.repairable != expected.repairable ||
        actual.commits != expectedCommitMask ||
        actual.configs != expectedConfigs ||
        actual.patterns != expectedPatterns ||
        actual.failure != expected.failure) {
        ++mismatches;
    }
}

Schedule scheduleFromMap(std::uint16_t validMap, unsigned salt)
{
    Schedule schedule{};
    for (unsigned sa = 0; sa < 4; ++sa) {
        for (unsigned slot = 0; slot < 4; ++slot) {
            const unsigned index = sa * 4U + slot;
            schedule[sa][slot].valid = (validMap & (1U << index)) != 0;
            schedule[sa][slot].pattern = ((salt + index) % 10U) + 1U;
        }
    }
    return schedule;
}

std::uint32_t nextRandom(std::uint32_t &state)
{
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

} // namespace

int main()
{
    unsigned legalPathCount = 0;
    for (unsigned tuple = 0; tuple < 256U; ++tuple) {
        const std::array<unsigned, 4> slots{{
            tuple & 3U, (tuple >> 2U) & 3U,
            (tuple >> 4U) & 3U, (tuple >> 6U) & 3U}};
        legalPathCount += legalPath(slots) ? 1U : 0U;
    }
    require(legalPathCount == 81U, "HYP02 static relation does not define 81 paths");

    unsigned exhaustiveMismatches = 0;
    for (unsigned validMap = 0; validMap < 65536U; ++validMap)
        compare(scheduleFromMap(static_cast<std::uint16_t>(validMap), validMap),
                exhaustiveMismatches);
    require(exhaustiveMismatches == 0,
            "RTL prefix legality differs from independent 81-path oracle");

    unsigned randomMismatches = 0;
    std::uint32_t randomState = 0x243f6a88U;
    for (unsigned vector = 0; vector < 1000U; ++vector) {
        const std::uint16_t validMap =
            static_cast<std::uint16_t>(nextRandom(randomState));
        compare(scheduleFromMap(validMap, nextRandom(randomState)), randomMismatches);
    }
    require(randomMismatches == 0,
            "RTL randomized result differs from independent 81-path oracle");

    const Schedule vector216 = scheduleFromMap(0x5a6cU, 216U);
    const OracleResult vector216Expected = oracle(vector216);
    const DutResult vector216Actual = run(vector216);
    require(vector216Actual.repairable == vector216Expected.repairable,
            "Vector216 boundary example differs from HYP02 oracle");

    std::cout << "HYP02_EARLY_ORACLE_INDEPENDENT=YES\n";
    std::cout << "HYP02_LEGAL_PATH_COUNT=" << legalPathCount << '\n';
    std::cout << "EXHAUSTIVE_VALIDITY_MAPS=65536\n";
    std::cout << "PREFIX_LEGALITY_MISMATCHES=" << exhaustiveMismatches << '\n';
    std::cout << "RANDOMIZED_VECTORS=1000\n";
    std::cout << "1000_VECTOR_MISMATCHES=" << randomMismatches << '\n';
    std::cout << "VECTOR216_HYP02_RESULT="
              << (vector216Expected.repairable ? "PASS" : "FAIL") << '\n';
    return 0;
}
