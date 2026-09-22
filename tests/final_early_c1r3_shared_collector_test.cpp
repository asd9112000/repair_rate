#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "SharedCollectorRecam.hpp"
#include "SimulationConfig.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

namespace
{
using namespace dynamic_spare;

constexpr std::uint64_t kSeed = 20260922;
constexpr std::size_t kGroups = 300;
constexpr std::array<std::uint64_t, 4> kLoads{{16, 20, 24, 28}};
constexpr std::array<std::size_t, 4> kP0{{1, 0, 3, 2}};
constexpr std::array<std::size_t, 4> kP1{{1, 3, 0, 2}};
constexpr std::array<std::size_t, 4> kAbcd{{0, 1, 2, 3}};
constexpr std::array<std::size_t, 4> kAbdc{{0, 1, 3, 2}};

void require(bool ok, const char *message)
{
    if (!ok) throw std::runtime_error(message);
}

SimulationConfig configFor(std::uint64_t faults, std::size_t groups = kGroups)
{
    SimulationConfig c;
    c.spareRows = c.spareColumns = 2;
    c.sharedRows = c.sharedColumns = 1;
    c.layout = GroupLayout::Grid2x2;
    c.topology = SharingTopology::Directional;
    c.solutionTakePolicy = SolutionTakePolicy::DirectionalV2Early;
    c.modifiers.maximumGroupBorrowedSpares = 1;
    c.usePaperCamReuseCapacity = true;
    c.faultCount = faults;
    c.simulationRuns = groups;
    c.randomSeed = kSeed;
    c.memoryRows = c.memoryColumns = 1024;
    c.faultCountModel = FaultCountModel::ModerateImbalance;
    c.faultSpatialModel = FaultSpatialModel::Mixed;
    return c;
}

std::vector<Fault> hybridVector(int base)
{
    return {{base,base+10,0,0,0,0,2}, {base+1,base+10,0,0,0,0,2},
            {base+2,base+8,0,0,0,0,2}, {base+3,base+8,0,0,0,0,2}};
}

RECAMSolverRequest request(int rows = 2, int columns = 1)
{
    RECAMSolverRequest r;
    r.subarrayId = 2;
    r.availableRows = rows;
    r.availableColumns = columns;
    r.provisionedRows = 3;
    r.provisionedColumns = 2;
    r.bufferCamEntries = 4;
    r.rowAddressWidthBits = r.columnAddressWidthBits = 10;
    return r;
}

const CandidateRepairOption *candidate(const GroupRepairResult &group,
                                       std::size_t sa, std::size_t slot)
{
    const auto &attempt = group.attemptsBySubarray.at(sa).at(slot);
    if (!attempt.repairSuccess || attempt.validCandidateOptions.empty()) return nullptr;
    return &*std::min_element(attempt.validCandidateOptions.begin(),
        attempt.validCandidateOptions.end(), [](const auto &a, const auto &b)
        { return a.candidateIndex < b.candidateIndex; });
}

constexpr std::array<std::uint8_t,4> kOwner{{1,4,8,2}};
constexpr std::array<std::uint8_t,4> kBorrower{{4,2,1,8}};

std::uint8_t claim(std::size_t sa, const CandidateRepairOption &c)
{
    const bool owner = (sa == 0 || sa == 3) ? c.usedRows >= 2 : c.usedColumns >= 2;
    const bool borrower = (sa == 0 || sa == 3) ? c.usedColumns > 2 : c.usedRows > 2;
    return (owner ? kOwner[sa] : 0) | (borrower ? kBorrower[sa] : 0);
}

struct Evaluation
{
    bool success = false;
    std::array<int,4> slots{{-1,-1,-1,-1}};
    std::vector<std::pair<std::string, std::uint8_t>> contexts;
};

// Test-local token replay.  It uses neither the production selection helper
// nor the shared collector and makes the intended four-bit transition explicit.
Evaluation evaluate(const GroupRepairResult &group, const std::array<std::size_t,4> &order,
                    const std::array<std::size_t,4> &priority)
{
    Evaluation out;
    std::uint8_t available = 0x0f;
    std::size_t borrows = 0;
    for (std::size_t sa : order)
    {
        std::string history;
        for (std::size_t slot : priority)
        {
            const auto *c = candidate(group, sa, slot);
            if (c == nullptr)
            {
                history += (history.empty() ? "" : ";") +
                    std::string(slot == 1 ? "R" : slot == 0 ? "L" :
                        slot == 3 ? "RB" : "B") + "=I";
                continue;
            }
            const std::uint8_t use = claim(sa, *c);
            const bool borrowsNow = (use & kBorrower[sa]) != 0;
            if ((available & use) != use || (borrowsNow && borrows == 1))
            {
                history += (history.empty() ? "" : ";") +
                    std::string(slot == 1 ? "R" : slot == 0 ? "L" :
                        slot == 3 ? "RB" : "B") + "=X";
                continue;
            }
            const std::string role = slot == 1 ? "R" : slot == 0 ? "L" :
                slot == 3 ? "RB" : "B";
            out.contexts.push_back({std::string(1, char('A' + sa)) + ":" +
                role + ":" + history, use});
            available = static_cast<std::uint8_t>(available & ~use);
            if (borrowsNow) ++borrows;
            out.slots[sa] = static_cast<int>(slot);
            break;
        }
        if (out.slots[sa] < 0) return out;
    }
    out.success = true;
    return out;
}

std::array<int,4> productionSlots(const GroupRepairResult &group)
{
    std::array<int,4> slots{{-1,-1,-1,-1}};
    for (const auto &entry : group.v2DecisionTrace)
        if (entry.selected) slots[entry.subarray] = static_cast<int>(entry.roleSlot);
    return slots;
}

struct Counts { std::size_t both = 0, neither = 0, right = 0, left = 0; };
void pair(Counts &c, bool left, bool right)
{
    if (left && right) ++c.both;
    else if (!left && !right) ++c.neither;
    else if (right) ++c.right;
    else ++c.left;
}

void run()
{
    const std::filesystem::path root = "tmp/date2026/final_early_c1r3";
    for (const char *dir : {"model_b2_regression", "vector216", "analogous_16", "policy_4way", "claim_context_recheck"})
        std::filesystem::create_directories(root / dir);
    std::ofstream oracle(root / "model_b2_regression/oracle.txt");
    std::ofstream vector(root / "vector216/result.txt");
    std::ofstream replay(root / "analogous_16/replay.csv");
    std::ofstream policy(root / "policy_4way/summary.csv");
    require(oracle.good() && vector.good() && replay.good() && policy.good(), "cannot write C1R3 temp evidence");

    // Independent hand-derived 2R1C reference.  It checks retained Hybrid
    // records, PatternID 2, and the whole row-major matrix rather than asking
    // the production implementation how to reject a record.
    const std::vector<bool> hybridMatrix{true,true,false, false,true,false, true,true,false};
    const auto h = solveSharedCollectorConfigSet(hybridVector(100), {request()}).front();
    require(h.repairSuccess && h.successfulCandidateIndex == 1 && h.sharedCollectorMatrixBits == hybridMatrix,
            "independent Hybrid shared-state oracle mismatch");
    const std::vector<Fault> mustFaults{{300,400,0,0,0,0,2}, {300,401,0,0,0,0,2}, {300,402,0,0,0,0,2}};
    const std::vector<bool> mustMatrix{true,true,true, false,false,false, false,false,false};
    const auto m = solveSharedCollectorConfigSet(mustFaults, {request()}).front();
    require(m.repairSuccess && m.sharedCollectorMatrixBits == mustMatrix,
            "independent RowMust shared-state oracle mismatch");
    for (int i = 0; i < 128; ++i)
    {
        const auto random = solveSharedCollectorConfigSet(hybridVector(1000 + 10*i), {request()}).front();
        require(random.repairSuccess && random.successfulCandidateIndex == 1 && random.sharedCollectorMatrixBits == hybridMatrix,
                "independent random Model-B2 oracle mismatch");
    }
    oracle << "independent_vectors=130\nconfig_valid_pattern_slot_must_matrix=PASS\n";

    DynamicRepairSimulator simulator;
    const auto vcfg = configFor(16);
    DynamicFaultGenerator vgen(vcfg);
    const GroupRepairResult v216 = simulator.run(vgen.generate(216), vcfg, 216, true);
    const auto &cr = v216.attemptsBySubarray[2].at(1);
    require(cr.repairSuccess && cr.successfulCandidateIndex == 1 && productionSlots(v216)[2] == 1,
            "Vector216 C:R must be valid PatternID 2 and selected before L");
    vector << "C_R_VALID=1\nC_R_PATTERN=2\nC_SELECTED_SLOT=R\nC_L_REACHED=0\n";

    struct Replay { int sa; std::uint64_t load; std::size_t group; };
    constexpr std::array<Replay,16> known{{{2,16,7},{2,16,155},{2,16,171},{2,16,196},{2,16,216},{2,20,9},{1,20,108},{1,20,171},{2,20,177},{2,20,190},{1,20,216},{1,20,219},{1,20,220},{1,20,290},{1,24,219},{1,28,35}}};
    replay << "sa,f_group,group,r_valid,pattern\n";
    for (const auto item : known)
    {
        const auto cfg = configFor(item.load);
        DynamicFaultGenerator gen(cfg);
        const auto result = simulator.run(gen.generate(item.group), cfg, item.group, true);
        const auto &r = result.attemptsBySubarray[item.sa].at(1);
        require(r.repairSuccess, "analogous C1R R case remains invalid");
        replay << char('A' + item.sa) << ',' << item.load << ',' << item.group << ',' << r.repairSuccess << ',' << r.successfulCandidateIndex.value_or(99) + 1 << '\n';
    }

    policy << "f_group,c0_abcd_p0,c1_abcd_p1,c2_abdc_p0,c3_abdc_p1,p1_only_abcd,p0_only_abcd,p1_only_abdc,p0_only_abdc,abdc_only_p0,abcd_only_p0,abdc_only_p1,abcd_only_p1\n";
    std::map<std::string, std::set<std::uint8_t>> contexts;
    for (std::uint64_t load : kLoads)
    {
        const auto cfg = configFor(load);
        DynamicFaultGenerator gen(cfg);
        Counts priorityAbcd, priorityAbdc, orderP0, orderP1;
        std::array<std::size_t,4> pass{};
        for (std::size_t group = 0; group < kGroups; ++group)
        {
            const auto source = simulator.run(gen.generate(group), cfg, group, true);
            const auto c0 = evaluate(source, kAbcd, kP0);
            const auto c1 = evaluate(source, kAbcd, kP1);
            const auto c2 = evaluate(source, kAbdc, kP0);
            const auto c3 = evaluate(source, kAbdc, kP1);
            require(c0.success == source.groupRepairSuccess && c0.slots == productionSlots(source),
                    "independent P0 ABCD replay differs from production");
            const std::array<Evaluation,4> cases{{c0,c1,c2,c3}};
            for (std::size_t i = 0; i < 4; ++i)
            {
                if (cases[i].success) ++pass[i];
                for (const auto &context : cases[i].contexts)
                    contexts[context.first].insert(context.second);
            }
            pair(priorityAbcd,c0.success,c1.success); pair(priorityAbdc,c2.success,c3.success);
            pair(orderP0,c0.success,c2.success); pair(orderP1,c1.success,c3.success);
        }
        policy << load;
        for (auto n : pass) policy << ',' << std::fixed << std::setprecision(6) << 100.0*n/kGroups;
        policy << ',' << priorityAbcd.right << ',' << priorityAbcd.left << ',' << priorityAbdc.right << ',' << priorityAbdc.left
               << ',' << orderP0.right << ',' << orderP0.left << ',' << orderP1.right << ',' << orderP1.left << '\n';
    }
    std::size_t ambiguities = 0;
    for (const auto &context : contexts)
        if (context.second.size() > 1) ++ambiguities;
    std::ofstream claim(root / "claim_context_recheck/summary.txt");
    claim << "contexts=" << contexts.size() << '\n'
          << "B_L_R_I_replayed=" << (contexts.count("B:L:R=I") != 0) << '\n'
          << "C_L_R_I_replayed=" << (contexts.count("C:L:R=I") != 0) << '\n'
          << "resource_action_ambiguities=" << ambiguities << '\n'
          << "vector216_reaches_L=NO\n";
    std::cout << "FINAL_EARLY_C1R3 ORACLE=PASS VECTOR216=PASS ANALOGOUS_16=PASS\n";
}
} // namespace

int main()
{
    try { run(); }
    catch (const std::exception &error) { std::cerr << "FINAL_EARLY_C1R3 FAIL: " << error.what() << '\n'; return 1; }
    return 0;
}
