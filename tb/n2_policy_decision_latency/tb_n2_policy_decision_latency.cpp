#include "Vtb_n2_policy_decision_latency_top.h"
#include "verilated.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr unsigned kSeed = 20260922U;
constexpr int kClockNs = 20;
constexpr int kPreflightVectors = 1000;
constexpr int kFormalVectors = 10000;
constexpr int kPivotSlots = 5;
constexpr int kTimeoutCycles = 64;
using Dut = Vtb_n2_policy_decision_latency_top;

struct Snapshot {
    std::uint8_t pivotValid = 0;
    std::array<std::uint16_t, kPivotSlots> rows{};
    std::array<std::uint16_t, kPivotSlots> cols{};
    std::uint8_t rowGt1 = 0, rowGt2 = 0, rowGt3 = 0;
    std::uint8_t colGt1 = 0, colGt2 = 0, colGt3 = 0;
    struct Hybrid {
        bool valid = false;
        std::uint8_t pointer = 0;
        bool differingIsRow = false;
        std::uint16_t differing = 0;
    };
    std::array<Hybrid, 7> hybrids{};
    bool overflow = false;
};

struct Record {
    unsigned id = 0;
    bool earlyRepairable = false, groupRepairable = false;
    std::optional<int> earlyFirstCommit, earlyFinalCommit;
    int earlyDone = -1, earlyCommitCount = 0, groupDone = -1;
    std::uint16_t earlyConfig = 0, earlyPattern = 0;
    std::uint16_t groupConfig = 0, groupPattern = 0;
};

struct Stats { std::size_t count; int min, p95, max; double mean, median; };

[[noreturn]] void fail(const std::string& message) { throw std::runtime_error(message); }
void require(bool condition, const std::string& message) { if (!condition) fail(message); }
bool bit(std::uint8_t value, int index) { return ((value >> index) & 1U) != 0U; }
void clear(WData* values, int words) { for (int index = 0; index < words; ++index) values[index] = 0U; }
void put(WData* values, int offset, int width, std::uint32_t value) {
    for (int bitIndex = 0; bitIndex < width; ++bitIndex) {
        const int target = offset + bitIndex;
        const WData mask = WData{1U} << (target % 32);
        if ((value >> bitIndex) & 1U) values[target / 32] |= mask;
        else values[target / 32] &= ~mask;
    }
}
void falling(Dut& dut) { dut.clk_i = 0; dut.eval(); }
void rising(Dut& dut) { dut.clk_i = 1; dut.eval(); }
void tick(Dut& dut) { falling(dut); rising(dut); }

Snapshot makeSnapshot(unsigned id, bool forceOverflow = false) {
    std::mt19937 rng(kSeed + id * 747796405U);
    Snapshot snapshot;
    const int count = static_cast<int>(rng() % 6U);
    snapshot.pivotValid = count == 0 ? 0U : static_cast<std::uint8_t>((1U << count) - 1U);
    std::set<std::uint16_t> usedRows, usedCols;
    for (int pivot = 0; pivot < count; ++pivot) {
        do snapshot.rows[pivot] = static_cast<std::uint16_t>(rng() & 0x1ffU);
        while (!usedRows.insert(snapshot.rows[pivot]).second);
        do snapshot.cols[pivot] = static_cast<std::uint16_t>(rng() & 0x1fffU);
        while (!usedCols.insert(snapshot.cols[pivot]).second);
        const int rowFaults = 1 + static_cast<int>(rng() % 4U);
        const int colFaults = 1 + static_cast<int>(rng() % 4U);
        if (rowFaults >= 2) snapshot.rowGt1 |= std::uint8_t{1U} << pivot;
        if (rowFaults >= 3) snapshot.rowGt2 |= std::uint8_t{1U} << pivot;
        if (rowFaults >= 4) snapshot.rowGt3 |= std::uint8_t{1U} << pivot;
        if (colFaults >= 2) snapshot.colGt1 |= std::uint8_t{1U} << pivot;
        if (colFaults >= 3) snapshot.colGt2 |= std::uint8_t{1U} << pivot;
        if (colFaults >= 4) snapshot.colGt3 |= std::uint8_t{1U} << pivot;
    }
    if (count >= 2) {
        const int hybridCount = static_cast<int>(rng() % 8U);
        for (int hybrid = 0; hybrid < hybridCount; ++hybrid) {
            const int pointer = static_cast<int>(rng() % static_cast<unsigned>(count));
            int other = static_cast<int>(rng() % static_cast<unsigned>(count - 1));
            if (other >= pointer) ++other;
            const bool differingIsRow = (rng() & 1U) != 0U;
            snapshot.hybrids[hybrid] = {true, static_cast<std::uint8_t>(pointer), differingIsRow,
                differingIsRow ? snapshot.rows[other] : snapshot.cols[other]};
        }
    }
    snapshot.overflow = forceOverflow || id % 29U == 0U;
    return snapshot;
}

void validateSnapshot(const Snapshot& snapshot) {
    bool gap = false;
    std::set<std::uint16_t> rows, cols;
    for (int pivot = 0; pivot < kPivotSlots; ++pivot) {
        if (!bit(snapshot.pivotValid, pivot)) { gap = true; continue; }
        require(!gap, "semantic validity: pivot bitmap is not a compact prefix");
        require(rows.insert(snapshot.rows[pivot]).second, "semantic validity: duplicate pivot row");
        require(cols.insert(snapshot.cols[pivot]).second, "semantic validity: duplicate pivot physical column");
    }
    const std::uint8_t inactive = static_cast<std::uint8_t>(~snapshot.pivotValid) & 0x1fU;
    require(((snapshot.rowGt1 | snapshot.rowGt2 | snapshot.rowGt3 | snapshot.colGt1 |
              snapshot.colGt2 | snapshot.colGt3) & inactive) == 0U,
            "semantic validity: threshold bit outside active pivot prefix");
    require((snapshot.rowGt3 & ~snapshot.rowGt2) == 0U &&
            (snapshot.rowGt2 & ~snapshot.rowGt1) == 0U &&
            (snapshot.colGt3 & ~snapshot.colGt2) == 0U &&
            (snapshot.colGt2 & ~snapshot.colGt1) == 0U,
            "semantic validity: non-monotonic threshold masks");
    int activePivots = 0;
    while (activePivots < kPivotSlots && bit(snapshot.pivotValid, activePivots)) ++activePivots;
    bool hybridGap = false;
    for (const Snapshot::Hybrid& hybrid : snapshot.hybrids) {
        if (!hybrid.valid) { hybridGap = true; continue; }
        require(!hybridGap, "semantic validity: hybrid records are not a compact prefix");
        require(activePivots > 0 && hybrid.pointer < activePivots,
                "semantic validity: hybrid pointer is outside the active dictionary");
        require(hybrid.differing <= (hybrid.differingIsRow ? 0x1ffU : 0x1fffU),
                "semantic validity: hybrid differing address has invalid width");
    }
}

void driveSnapshot(Dut& dut, const Snapshot& snapshot) {
    dut.pivot_valid_i = snapshot.pivotValid;
    dut.pivot_rows_flat_i = 0;
    clear(dut.pivot_cols_flat_i, 3); clear(dut.hybrid_differing_flat_i, 3);
    for (int pivot = 0; pivot < kPivotSlots; ++pivot) {
        dut.pivot_rows_flat_i |= static_cast<QData>(snapshot.rows[pivot]) << (9 * pivot);
        put(dut.pivot_cols_flat_i, 13 * pivot, 13, snapshot.cols[pivot]);
    }
    dut.row_gt1_i = snapshot.rowGt1; dut.row_gt2_i = snapshot.rowGt2; dut.row_gt3_i = snapshot.rowGt3;
    dut.col_gt1_i = snapshot.colGt1; dut.col_gt2_i = snapshot.colGt2; dut.col_gt3_i = snapshot.colGt3;
    dut.hybrid_valid_i = 0; dut.hybrid_pointer_flat_i = 0; dut.hybrid_descriptor_i = 0;
    for (int hybrid = 0; hybrid < 7; ++hybrid) {
        const Snapshot::Hybrid& entry = snapshot.hybrids[hybrid];
        if (!entry.valid) continue;
        dut.hybrid_valid_i |= std::uint8_t{1U} << hybrid;
        dut.hybrid_pointer_flat_i |= static_cast<std::uint32_t>(entry.pointer) << (3 * hybrid);
        if (entry.differingIsRow) dut.hybrid_descriptor_i |= std::uint8_t{1U} << hybrid;
        put(dut.hybrid_differing_flat_i, 13 * hybrid, 13, entry.differing);
    }
    dut.conventional_overflow_i = snapshot.overflow ? 1U : 0U;
}

void reset(Dut& dut, const Snapshot& snapshot) {
    dut.start_i = 0; driveSnapshot(dut, snapshot); dut.rst_ni = 0; tick(dut); dut.rst_ni = 1; tick(dut);
}

Record measure(Dut& dut, const Snapshot& snapshot, unsigned id, std::ostream* trace = nullptr,
               const std::string& label = "") {
    validateSnapshot(snapshot); reset(dut, snapshot); Record record; record.id = id;
    dut.start_i = 1; falling(dut);
    require(dut.early_busy_o == 0U && dut.group_busy_o == 0U, "start_accept requires idle controllers");
    rising(dut); dut.start_i = 0;
    require(dut.early_busy_o != 0U && dut.group_busy_o != 0U, "start was not accepted by both controllers");
    if (trace) *trace << label << ",0,0,0,0,start_accept\n";

    for (int edge = 1; edge <= kTimeoutCycles; ++edge) {
        falling(dut);
        const bool earlyCommit = dut.early_commit_valid_o != 0U;
        if (earlyCommit) {
            const unsigned sa = dut.early_solution_sa_o;
            require(sa < 4U, "EARLY commit SA out of range");
            ++record.earlyCommitCount;
            if (!record.earlyFirstCommit) record.earlyFirstCommit = edge;
            record.earlyFinalCommit = edge;
            record.earlyConfig &= static_cast<std::uint16_t>(~(0x7U << (3U * sa)));
            record.earlyConfig |= static_cast<std::uint16_t>(dut.early_solution_config_o) << (3U * sa);
            record.earlyPattern &= static_cast<std::uint16_t>(~(0xfU << (4U * sa)));
            record.earlyPattern |= static_cast<std::uint16_t>(dut.early_solution_pattern_o) << (4U * sa);
        }
        rising(dut);
        if (dut.early_done_o && record.earlyDone < 0) {
            record.earlyDone = edge; record.earlyRepairable = dut.early_repairable_o != 0U;
        }
        if (dut.group_done_o && record.groupDone < 0) {
            record.groupDone = edge; record.groupRepairable = dut.group_repairable_o != 0U;
            record.groupConfig = dut.group_selected_config_flat_o;
            record.groupPattern = dut.group_selected_pattern_flat_o;
            require(record.groupRepairable ? dut.group_commit_valid_o == 0xfU : dut.group_commit_valid_o == 0U,
                    "GROUP terminal commit mask violates atomic result contract");
        }
        if (trace) *trace << label << ',' << edge << ',' << earlyCommit << ',' << unsigned(dut.early_done_o)
                          << ',' << unsigned(dut.group_done_o) << ",edge\n";
        if (record.earlyDone >= 0 && record.groupDone >= 0) break;
    }
    require(record.earlyDone >= 0 && record.groupDone >= 0, "policy controller timeout");
    require(record.earlyCommitCount <= 4, "EARLY emitted more than four commits");
    if (record.earlyRepairable) require(record.earlyCommitCount == 4, "repairable EARLY omitted a commit");
    else record.earlyFinalCommit.reset();
    require(record.groupDone == 17, "observed GROUP done latency is not 17 cycles");
    const auto earlyConfigTerminal = dut.early_solution_config_o;
    const auto earlyPatternTerminal = dut.early_solution_pattern_o;
    const auto groupConfigTerminal = dut.group_selected_config_flat_o;
    const auto groupPatternTerminal = dut.group_selected_pattern_flat_o;
    tick(dut);
    require(dut.early_solution_config_o == earlyConfigTerminal && dut.early_solution_pattern_o == earlyPatternTerminal,
            "EARLY selected outputs changed after terminal state");
    require(dut.group_selected_config_flat_o == groupConfigTerminal && dut.group_selected_pattern_flat_o == groupPatternTerminal,
            "GROUP selected outputs changed after terminal state");
    return record;
}

std::string csv(const std::optional<int>& value) { return value ? std::to_string(*value) : "NA"; }
Stats statistics(std::vector<int> values) {
    require(!values.empty(), "empty statistics population"); std::sort(values.begin(), values.end());
    long long sum = 0; for (int value : values) sum += value;
    const std::size_t middle = values.size() / 2;
    return {values.size(), values.front(), values.at(static_cast<std::size_t>(std::ceil(values.size() * 0.95)) - 1U),
            values.back(), static_cast<double>(sum) / values.size(),
            values.size() % 2U ? static_cast<double>(values[middle]) :
            (static_cast<double>(values[middle - 1]) + values[middle]) / 2.0};
}

std::vector<int> values(const std::vector<Record>& records, const std::string& policy,
                        const std::string& metric, const std::string& population) {
    std::vector<int> result;
    for (const Record& record : records) {
        const bool repairable = policy == "EARLY" ? record.earlyRepairable : record.groupRepairable;
        if (population == "REPAIRABLE" && !repairable) continue;
        if (population == "UNREPAIRABLE" && repairable) continue;
        if (policy == "EARLY" && metric == "L_FIRST_COMMIT" && record.earlyFirstCommit) result.push_back(*record.earlyFirstCommit);
        if (policy == "EARLY" && metric == "L_FINAL_COMMIT" && record.earlyFinalCommit) result.push_back(*record.earlyFinalCommit);
        if (policy == "EARLY" && metric == "L_DONE") result.push_back(record.earlyDone);
        if (policy == "GROUP" && metric == "L_DONE") result.push_back(record.groupDone);
    }
    return result;
}

void summaryRow(std::ofstream& stream, const std::string& scope, const std::string& policy,
                const std::string& metric, const std::string& population, const std::vector<int>& data) {
    stream << scope << ',' << policy << ',' << metric << ',' << population << ',';
    if (data.empty()) { stream << "0,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA\n"; return; }
    const Stats s = statistics(data); stream << std::fixed << std::setprecision(4) << s.count << ',' << s.min << ','
        << s.mean << ',' << s.median << ',' << s.p95 << ',' << s.max << ',' << s.min * kClockNs << ','
        << s.mean * kClockNs << ',' << s.median * kClockNs << ',' << s.p95 * kClockNs << ',' << s.max * kClockNs << '\n';
}

void writeRaw(const std::filesystem::path& path, const std::vector<Record>& records) {
    std::ofstream stream(path); require(stream.good(), "cannot create " + path.string());
    stream << "vector_id,seed,early_repairable,group_repairable,early_start_cycle,early_first_commit_cycle,early_final_commit_cycle,early_done_cycle,early_l_first_commit,early_l_final_commit,early_l_done,early_commit_count,group_start_cycle,group_done_cycle,group_l_done,delta_done,early_selected_config_flat,early_selected_pattern_flat,group_selected_config_flat,group_selected_pattern_flat\n";
    for (const Record& record : records) stream << record.id << ',' << kSeed << ',' << record.earlyRepairable << ','
        << record.groupRepairable << ",0," << csv(record.earlyFirstCommit) << ',' << csv(record.earlyFinalCommit) << ','
        << record.earlyDone << ',' << csv(record.earlyFirstCommit) << ',' << csv(record.earlyFinalCommit) << ','
        << record.earlyDone << ',' << record.earlyCommitCount << ",0," << record.groupDone << ',' << record.groupDone << ','
        << record.groupDone - record.earlyDone << ',' << record.earlyConfig << ',' << record.earlyPattern << ','
        << record.groupConfig << ',' << record.groupPattern << '\n';
}

void writeResults(const std::filesystem::path& root, const std::vector<Record>& preflight,
                  const std::vector<Record>& formal) {
    writeRaw(root / "preflight_raw_latency.csv", preflight); writeRaw(root / "raw_latency.csv", formal);
    std::ofstream summary(root / "summary.csv"); require(summary.good(), "cannot create summary.csv");
    summary << "scope,policy,metric,population,sample_count,min_cycles,mean_cycles,median_cycles,p95_cycles,max_cycles,min_ns,mean_ns,median_ns,p95_ns,max_ns\n";
    for (const auto& stage : {std::pair<std::string, const std::vector<Record>&>{"PREFLIGHT", preflight}, {"FORMAL", formal}}) {
        for (const std::string& population : {"ALL", "REPAIRABLE", "UNREPAIRABLE"}) {
            summaryRow(summary, stage.first, "EARLY", "L_FIRST_COMMIT", population, values(stage.second, "EARLY", "L_FIRST_COMMIT", population));
            summaryRow(summary, stage.first, "EARLY", "L_FINAL_COMMIT", population, values(stage.second, "EARLY", "L_FINAL_COMMIT", population));
            summaryRow(summary, stage.first, "EARLY", "L_DONE", population, values(stage.second, "EARLY", "L_DONE", population));
            summaryRow(summary, stage.first, "GROUP", "L_DONE", population, values(stage.second, "GROUP", "L_DONE", population));
        }
        std::vector<int> allDelta, bothRepairable;
        for (const Record& record : stage.second) { allDelta.push_back(record.groupDone - record.earlyDone); if (record.earlyRepairable && record.groupRepairable) bothRepairable.push_back(record.groupDone - record.earlyDone); }
        summaryRow(summary, stage.first, "PAIRED", "GROUP_MINUS_EARLY_L_DONE", "ALL", allDelta);
        summaryRow(summary, stage.first, "PAIRED", "GROUP_MINUS_EARLY_L_DONE", "BOTH_REPAIRABLE", bothRepairable);
    }
    const auto countOutcome = [&formal](bool early, bool group) { return std::count_if(formal.begin(), formal.end(), [early, group](const Record& r) { return r.earlyRepairable == early && r.groupRepairable == group; }); };
    for (const auto& item : {std::pair<const char*, int>{"BOTH_REPAIRABLE", countOutcome(true, true)}, {"EARLY_ONLY", countOutcome(true, false)}, {"GROUP_ONLY", countOutcome(false, true)}, {"BOTH_UNREPAIRABLE", countOutcome(false, false)}})
        summary << "FORMAL,OUTCOME," << item.first << ",ALL," << item.second << ",NA,NA,NA,NA,NA,NA,NA,NA,NA,NA\n";
    std::ofstream readme(root / "README.md"); require(readme.good(), "cannot create README.md");
    readme << "# N2 final-snapshot policy-decision latency\n\nThese values measure final-snapshot policy-decision latency. They do not include BIST, raw-fault collection, online-CAM allocation, or device-level scheduling.\n\n"
           << "- Preflight/formal vectors: 1000 / 10000\n- Seed: " << kSeed << "\n- Clock: 20.0 ns\n- Counting: accepted `start_i && !busy_o` rising edge is cycle 0; later events use `event edge - 0`.\n"
           << "- Generator: constrained extension of `tb/recam/recam_shared_config_analyzer_test.cpp`: compact pivot and hybrid prefixes, unique 9-bit rows and 13-bit physical columns, nested threshold masks, valid in-dictionary hybrid cross-edges, and explicit collector-overflow terminal snapshots.\n"
           << "- EARLY and GROUP receive the same held-final snapshot per vector. RECAM is combinational: `RECAM_POLICY_DECISION_CYCLES=N/A_COMBINATIONAL`.\n"
           << "- GROUP `sa_commit_valid_o` is coincident with terminal `done_o`; it is not an EARLY-style early commit event.\n";
}

std::vector<Record> runStage(Dut& dut, int count, const char* name) {
    std::vector<Record> records; records.reserve(count);
    for (int index = 0; index < count; ++index) records.push_back(measure(dut, makeSnapshot(static_cast<unsigned>(index)), static_cast<unsigned>(index)));
    std::cout << name << ' ' << count << '/' << count << " PASS\n"; return records;
}

void directed(Dut& dut, std::ofstream& trace) {
    const Record fast = measure(dut, Snapshot{}, 0, &trace, "FAST_SUCCESS");
    require(fast.earlyRepairable && fast.groupRepairable, "all-local directed case must repair");
    const Record failure = measure(dut, makeSnapshot(1, true), 1, &trace, "FAILURE_OVERFLOW");
    require(!failure.earlyRepairable && !failure.groupRepairable, "overflow directed case must fail");
    Record slow = fast;
    for (unsigned id = 2; id < 2002; ++id) { const Record candidate = measure(dut, makeSnapshot(id), id); if (candidate.earlyRepairable && candidate.earlyDone > slow.earlyDone) slow = candidate; }
    const Record traceSlow = measure(dut, makeSnapshot(slow.id), slow.id, &trace, "MAX_OBSERVED_EARLY_SUCCESS");
    require(traceSlow.earlyDone == slow.earlyDone, "slow directed trace is not reproducible");
    std::cout << "DIRECTED FAST early=" << fast.earlyDone << " GROUP=" << fast.groupDone << '\n';
    std::cout << "DIRECTED FAILURE early=" << failure.earlyDone << " GROUP=" << failure.groupDone << '\n';
    std::cout << "DIRECTED MAX_OBSERVED_EARLY vector=" << slow.id << " early=" << slow.earlyDone << " GROUP=" << slow.groupDone << '\n';
}
}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv); const std::filesystem::path root = "results/date2026/latency/n2_policy_decision";
    try {
        std::filesystem::create_directories(root); std::ofstream trace(root / "directed_timing_trace.csv");
        require(trace.good(), "cannot create directed timing trace"); trace << "label,edge,early_commit_before_edge,early_done_after_edge,group_done_after_edge,event\n";
        Dut dut; directed(dut, trace); const auto preflight = runStage(dut, kPreflightVectors, "PREFLIGHT"); const auto formal = runStage(dut, kFormalVectors, "FORMAL"); writeResults(root, preflight, formal);
        std::cout << "N2_POLICY_DECISION_LATENCY PASS seed=" << kSeed << "\n";
    } catch (const std::exception& error) { std::cerr << "N2_POLICY_DECISION_LATENCY FAIL: " << error.what() << '\n'; return 1; }
    return 0;
}
