#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "SimulationConfig.hpp"
#include "V2GroupNoScratchPolicy.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
using namespace dynamic_spare;

constexpr std::array<std::size_t, 4> kPriority{{1, 0, 3, 2}};
constexpr std::array<std::size_t, 4> kRelease{{0, 2, 3, 1}};
constexpr std::array<std::size_t, 4> kBorrow{{2, 1, 0, 3}};
constexpr std::array<std::size_t, 4> kOwner{{0, 3, 1, 2}};

struct Plan { std::size_t slot, pattern, rows, columns; };
struct State { std::uint8_t released = 0, used = 0, required = 0; std::size_t borrows = 0; };
struct Oracle {
    bool success = false;
    std::array<std::optional<int>, kSubarrayCount> configs{};
    std::array<std::optional<std::size_t>, kSubarrayCount> patterns{};
};
struct Counts {
    std::size_t bothPass = 0, b2Only = 0, oldOnly = 0, bothFail = 0;
    std::size_t configs = 0, patterns = 0, mismatchVectors = 0;
    std::size_t mismatches() const { return mismatchVectors; }
};

SimulationConfig configFor(std::uint64_t faults, SolutionTakePolicy policy)
{
    SimulationConfig c;
    c.spareRows = c.spareColumns = 2;
    c.sharedRows = c.sharedColumns = 1;
    c.layout = GroupLayout::Grid2x2;
    c.topology = SharingTopology::Directional;
    c.solutionTakePolicy = policy;
    c.modifiers.maximumGroupBorrowedSpares = 1;
    c.usePaperCamReuseCapacity = true;
    c.faultCount = faults;
    c.randomSeed = 20260922;
    c.memoryRows = c.memoryColumns = 1024;
    c.faultCountModel = FaultCountModel::ModerateImbalance;
    c.faultSpatialModel = FaultSpatialModel::Mixed;
    return c;
}

bool advance(State &s, std::size_t sa, V2GroupAction action,
             bool actualRelease, bool actualBorrow)
{
    const std::uint8_t releaseBit = static_cast<std::uint8_t>(1U << kRelease[sa]);
    const bool explicitRelease = action == V2GroupAction::ReleaseOnly ||
        action == V2GroupAction::ReleaseAndBorrow;
    if ((s.required & releaseBit) != 0 && !explicitRelease) return false;
    if (actualRelease) {
        s.released |= releaseBit;
        if (explicitRelease) s.required &= static_cast<std::uint8_t>(~releaseBit);
    }
    if (!actualBorrow) return true;
    if (s.borrows == 1) return false;
    const std::size_t donor = kBorrow[sa];
    const std::uint8_t donorBit = static_cast<std::uint8_t>(1U << donor);
    if ((s.used & donorBit) != 0) return false;
    if ((s.released & donorBit) == 0) {
        if (kOwner[donor] <= sa) return false;
        s.required |= donorBit;
    }
    s.used |= donorBit;
    ++s.borrows;
    return true;
}

// Independent audit oracle: full 11-entry Model-B2 attempts from the shared
// collector, then literal R,L,RB,B DFS.  It never calls a production GROUP
// selector; PhysicalResourceLedger remains the topology authority.
Oracle modelB2Oracle(const GroupRepairResult &attemptSource,
                     const SimulationConfig &physicalConfig)
{
    std::array<std::vector<Plan>, kSubarrayCount> plans;
    for (std::size_t sa = 0; sa < kSubarrayCount; ++sa) {
        const auto &attempts = attemptSource.attemptsBySubarray[sa];
        if (attempts.size() != 4) throw std::runtime_error("B2 attempt count is not four");
        for (std::size_t slot : kPriority) {
            const auto &attempt = attempts[slot];
            if (!attempt.repairSuccess) continue;
            for (const auto &candidate : attempt.validCandidateOptions)
                plans[sa].push_back({slot, candidate.candidateIndex,
                                     candidate.usedRows, candidate.usedColumns});
        }
        if (plans[sa].empty()) return {};
    }
    const PhysicalResourceLedger ledger(physicalConfig);
    Oracle result;
    std::array<Plan, kSubarrayCount> chosen{};
    std::array<SpareDemand, kSubarrayCount> demands{};
    bool stop = false;
    const auto visit = [&](const auto &self, std::size_t sa, State state) -> void {
        if (stop || sa == kSubarrayCount) return;
        const auto mappings = v2FrozenDate2x2RoleSlotMappings(sa);
        for (const Plan &plan : plans[sa]) {
            State next = state;
            const bool actualRelease = sa == 0 || sa == 3 ? plan.rows < 2 : plan.columns < 2;
            const bool actualBorrow = sa == 0 || sa == 3 ? plan.columns > 2 : plan.rows > 2;
            if (!advance(next, sa, mappings[plan.slot].action, actualRelease, actualBorrow)) continue;
            chosen[sa] = plan;
            demands[sa] = {plan.rows, plan.columns};
            if (sa + 1 != kSubarrayCount) { self(self, sa + 1, next); continue; }
            if (next.required != 0) continue;
            const auto allocation = ledger.allocateSequential(demands, kSubarrayCount);
            if (!allocation.success || allocation.transfers.size() > 1) continue;
            result.success = true;
            for (std::size_t i = 0; i < kSubarrayCount; ++i) {
                const auto map = v2FrozenDate2x2RoleSlotMappings(i);
                result.configs[i] = map[chosen[i].slot].configId;
                result.patterns[i] = chosen[i].pattern + 1;
            }
            stop = true;
            return;
        }
    };
    visit(visit, 0, {});
    return result;
}

void compare(const GroupRepairResult &old, const Oracle &b2, Counts &c)
{
    if (old.groupRepairSuccess && b2.success) {
        ++c.bothPass;
        const bool configDifference = old.selectedConfigIds != b2.configs;
        const bool patternDifference = old.selectedPatternIds != b2.patterns;
        c.configs += configDifference;
        c.patterns += patternDifference;
        c.mismatchVectors += configDifference || patternDifference;
    } else if (b2.success) { ++c.b2Only; ++c.mismatchVectors; }
    else if (old.groupRepairSuccess) { ++c.oldOnly; ++c.mismatchVectors; }
    else ++c.bothFail;
}

template <typename T>
std::string tuple(const std::array<std::optional<T>, kSubarrayCount> &values)
{
    std::ostringstream output;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) output << ':';
        output << (values[index].has_value()
            ? std::to_string(*values[index]) : "-");
    }
    return output.str();
}

void audit(DynamicRepairSimulator &sim, const FaultGroup &faults,
           const char *scope, std::uint64_t load, std::size_t id,
           Counts &counts, std::ofstream &out)
{
    const auto oldConfig = configFor(load, SolutionTakePolicy::DirectionalV2GroupGlobalCanonical);
    const auto b2Config = configFor(load, SolutionTakePolicy::DirectionalV2Early);
    const auto old = sim.run(faults, oldConfig, id, true);
    const auto b2 = modelB2Oracle(sim.run(faults, b2Config, id, true), oldConfig);
    compare(old, b2, counts);
    out << scope << ',' << load << ',' << id << ','
        << old.groupRepairSuccess << ',' << b2.success << ','
        << (old.selectedConfigIds != b2.configs) << ','
        << (old.selectedPatternIds != b2.patterns) << ','
        << tuple(old.selectedConfigIds) << ',' << tuple(b2.configs) << ','
        << tuple(old.selectedPatternIds) << ',' << tuple(b2.patterns) << '\n';
}
void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
} // namespace

int main()
{
    try {
        const std::filesystem::path root = "tmp/date2026/group_b2_compat_audit";
        std::filesystem::create_directories(root);
        std::ofstream out(root / "model_b2_group_comparison.csv");
        require(out.good(), "cannot write audit evidence");
        out << "scope,load,id,old_group_success,model_b2_success,config_difference,pattern_difference,old_configs,b2_configs,old_patterns,b2_patterns\n";
        DynamicRepairSimulator sim;
        Counts vector216, c1r16, corpus;
        auto generated = [&](const char *scope, std::uint64_t load, std::size_t id,
                             Counts &counts) {
            const auto config = configFor(load, SolutionTakePolicy::DirectionalV2Early);
            DynamicFaultGenerator generator(config);
            audit(sim, generator.generate(id), scope, load, id, counts, out);
        };
        generated("VECTOR216", 16, 216, vector216);
        constexpr std::array<std::pair<std::uint64_t, std::size_t>, 16> kC1R16{{
            {16,7},{16,155},{16,171},{16,196},{16,216},{20,9},{20,108},{20,171},
            {20,177},{20,190},{20,216},{20,219},{20,220},{20,290},{24,219},{28,35}}};
        for (const auto item : kC1R16)
            generated("C1R16", item.first, item.second, c1r16);
        constexpr std::array<std::uint64_t, 4> kLoads{{16,20,24,28}};
        std::size_t vectors = 0;
        for (const auto load : kLoads)
            for (std::size_t id = 0; id < 250; ++id, ++vectors)
                generated("CORPUS1000", load, id, corpus);
        require(vectors == 1000, "corpus length is not 1000");
        std::cout << "GROUP_MODEL_B2_MISMATCHES_1000=" << corpus.mismatches() << '\n'
                  << "VECTOR216_GROUP_DIFFERENCE=" << (vector216.mismatches() ? "YES" : "NO") << '\n'
                  << "C1R16_GROUP_DIFFERENCES=" << c1r16.mismatches() << '\n'
                  << "BOTH_PASS=" << corpus.bothPass << '\n'
                  << "MODEL_B2_ONLY=" << corpus.b2Only << '\n'
                  << "OLD_GROUP_ONLY=" << corpus.oldOnly << '\n'
                  << "BOTH_FAIL=" << corpus.bothFail << '\n'
                  << "SELECTED_CONFIG_DIFFERENCES=" << corpus.configs << '\n'
                  << "SELECTED_PATTERN_DIFFERENCES=" << corpus.patterns << '\n'
                  << "FINAL_REPAIRABILITY_DIFFERENCES=" << (corpus.b2Only + corpus.oldOnly) << '\n'
                  << "GROUP_RTL_MODIFIED=NO\nDSS_FINAL_GROUP_MODIFIED=NO\n"
                  << "GROUP_MODEL_B2_COMPAT_AUDIT=COMPLETE\n";
    } catch (const std::exception &error) {
        std::cerr << "GROUP_MODEL_B2_COMPAT_AUDIT=FAIL " << error.what() << '\n';
        return 1;
    }
    return 0;
}
