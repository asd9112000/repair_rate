#include "Vrecam_phase3b_verification_top.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int kMaxK = 5;
constexpr int kHybridEntries = 7;
constexpr int kRowAddrW = 9;
constexpr int kColAddrW = 5;
constexpr int kDiffAddrW = 9;
constexpr std::uint16_t kRowAddrMask = 0x1ffU;
constexpr std::uint16_t kColAddrMask = 0x01fU;
constexpr std::uint16_t kDiffAddrMask = 0x1ffU;

struct Hybrid {
    bool valid = false;
    std::uint8_t pointer = 0;
    bool descriptorRowDiff = false;
    std::uint16_t differing = 0;
};

struct State {
    std::uint8_t pivotValid = 0;
    std::array<std::uint16_t, kMaxK> rows{};
    std::array<std::uint16_t, kMaxK> cols{};
    std::uint8_t rowGt1 = 0, rowGt2 = 0, rowGt3 = 0;
    std::uint8_t colGt1 = 0, colGt2 = 0, colGt3 = 0;
    std::array<Hybrid, kHybridEntries> hybrids{};
    bool conventionalOverflow = false;
};

struct Result {
    std::uint16_t candidates = 0;
    std::uint8_t patternId = 0;
    bool solutionValid = false;
    bool repairable = false;
    bool dictionaryOverflow = false;
};

struct Geometry {
    int rows = 0;
    int cols = 0;
    bool transpose = false;
    bool valid = false;
};

[[noreturn]] void fail(const std::string &message)
{
    throw std::runtime_error(message);
}

void require(bool condition, const std::string &message)
{
    if (!condition)
        fail(message);
}

Geometry geometry(int cfg)
{
    switch (cfg) {
    case 0: return {2, 2, false, true};
    case 1: return {2, 1, false, true};
    case 2: return {3, 2, false, true};
    case 3: return {3, 1, false, true};
    case 4: return {2, 1, true, true};
    case 5: return {3, 2, true, true};
    case 6: return {3, 1, true, true};
    default: return {};
    }
}

int candidateCount(const Geometry &g)
{
    if (g.rows == 2 && g.cols == 1) return 3;
    if (g.rows == 2 && g.cols == 2) return 6;
    if (g.rows == 3 && g.cols == 1) return 4;
    if (g.rows == 3 && g.cols == 2) return 10;
    return 0;
}

std::uint8_t candidatePattern(const Geometry &g, int candidate)
{
    static constexpr std::array<std::uint8_t, 3> p21{{4, 2, 1}};
    static constexpr std::array<std::uint8_t, 6> p22{{12, 10, 6, 9, 5, 3}};
    static constexpr std::array<std::uint8_t, 4> p31{{8, 4, 2, 1}};
    static constexpr std::array<std::uint8_t, 10> p32{{24, 20, 12, 18, 10, 6, 17, 9, 5, 3}};
    if (g.rows == 2 && g.cols == 1) return p21.at(candidate);
    if (g.rows == 2 && g.cols == 2) return p22.at(candidate);
    if (g.rows == 3 && g.cols == 1) return p31.at(candidate);
    return p32.at(candidate);
}

bool bit(std::uint8_t value, int index)
{
    return ((value >> index) & 1U) != 0;
}

std::uint16_t columnWordToDiffering(std::uint16_t columnWord)
{
    return static_cast<std::uint16_t>(columnWord & kColAddrMask);
}

bool thresholdBit(const State &s, bool rowDimension, int threshold, int index)
{
    const std::uint8_t value = rowDimension
        ? (threshold == 1 ? s.rowGt1 : threshold == 2 ? s.rowGt2 : s.rowGt3)
        : (threshold == 1 ? s.colGt1 : threshold == 2 ? s.colGt2 : s.colGt3);
    return bit(value, index);
}

Result golden(int cfg, const State &s)
{
    const Geometry g = geometry(cfg);
    Result out;
    if (!g.valid)
        return out;

    const int k = g.rows + g.cols;
    int count = 0;
    std::array<std::uint16_t, kMaxK> rowDict{};
    std::array<std::uint16_t, kMaxK> colDict{};
    std::array<std::array<bool, kMaxK>, kMaxK> matrix{};
    for (int i = 0; i < kMaxK; ++i) {
        if (i < k && bit(s.pivotValid, i)) {
            rowDict[i] = g.transpose ? columnWordToDiffering(s.cols[i]) :
                static_cast<std::uint16_t>(s.rows[i] & kRowAddrMask);
            colDict[i] = g.transpose ? static_cast<std::uint16_t>(s.rows[i] & kRowAddrMask) :
                columnWordToDiffering(s.cols[i]);
            ++count;
        }
    }
    int rowCount = count;
    int colCount = count;
    for (int r = 0; r < k; ++r) {
        for (int c = 0; c < k; ++c) {
            const bool rowMust = r < count && thresholdBit(
                s, !g.transpose, g.cols, r);
            const bool colMust = c < count && thresholdBit(
                s, g.transpose, g.rows, c);
            matrix[r][c] = (r == c && r < count) || rowMust || colMust;
        }
    }

    for (const Hybrid &h : s.hybrids) {
        if (!h.valid)
            continue;
        const bool rowDiff = h.descriptorRowDiff != g.transpose;
        const std::uint16_t differing = h.descriptorRowDiff
            ? static_cast<std::uint16_t>(h.differing & kDiffAddrMask)
            : columnWordToDiffering(h.differing);
        if (h.pointer >= count) {
            out.dictionaryOverflow = true;
            continue;
        }
        bool match = false;
        int matchIndex = 0;
        if (rowDiff) {
            for (int i = 0; i < rowCount && !match; ++i) {
                if (rowDict[i] == differing) {
                    match = true;
                    matchIndex = i;
                }
            }
            if (!match && rowCount < k) {
                match = true;
                matchIndex = rowCount;
                rowDict[rowCount++] = differing;
            }
            if (match)
                matrix[matchIndex][h.pointer] = true;
            else
                // A full row dictionary turns an unseen row on a known
                // column into a required-column constraint.
                for (int r = 0; r < rowCount; ++r)
                    matrix[r][h.pointer] = true;
        } else {
            for (int i = 0; i < colCount && !match; ++i) {
                if (colDict[i] == differing) {
                    match = true;
                    matchIndex = i;
                }
            }
            if (!match && colCount < k) {
                match = true;
                matchIndex = colCount;
                colDict[colCount++] = differing;
            }
            if (match)
                matrix[h.pointer][matchIndex] = true;
            else
                // A full column dictionary turns an unseen column on a known
                // row into a required-row constraint.
                for (int c = 0; c < colCount; ++c)
                    matrix[h.pointer][c] = true;
        }
    }

    if (s.conventionalOverflow || out.dictionaryOverflow)
        return out;
    for (int candidate = 0; candidate < candidateCount(g); ++candidate) {
        const std::uint8_t pattern = candidatePattern(g, candidate);
        bool valid = true;
        for (int r = 0; r < k; ++r)
            for (int c = 0; c < k; ++c)
                if (matrix[r][c] && bit(pattern, r) && !bit(pattern, c))
                    valid = false;
        if (valid) {
            out.candidates |= static_cast<std::uint16_t>(1U << candidate);
            if (!out.solutionValid) {
                out.solutionValid = true;
                out.repairable = true;
                out.patternId = static_cast<std::uint8_t>(candidate + 1);
            }
        }
    }
    return out;
}

std::uint64_t packRows(const std::array<std::uint16_t, kMaxK> &values)
{
    std::uint64_t packed = 0;
    for (int i = 0; i < kMaxK; ++i)
        packed |= static_cast<std::uint64_t>(values[i] & kRowAddrMask) << (kRowAddrW * i);
    return packed;
}

std::uint32_t packColumns(const std::array<std::uint16_t, kMaxK> &values)
{
    std::uint32_t packed = 0;
    for (int i = 0; i < kMaxK; ++i)
        packed |= static_cast<std::uint32_t>(values[i] & kColAddrMask) << (kColAddrW * i);
    return packed;
}

std::uint64_t packDifferingAddresses(const State &s)
{
    std::uint64_t packed = 0;
    for (int i = 0; i < kHybridEntries; ++i) {
        const std::uint16_t differing = s.hybrids[i].descriptorRowDiff
            ? static_cast<std::uint16_t>(s.hybrids[i].differing & kDiffAddrMask)
            : columnWordToDiffering(s.hybrids[i].differing);
        packed |= static_cast<std::uint64_t>(differing) << (kDiffAddrW * i);
    }
    return packed;
}

void drive(Vrecam_phase3b_verification_top &dut, int cfg, const State &s)
{
    dut.config_id_i = cfg;
    dut.pivot_valid_i = s.pivotValid;
    dut.pivot_rows_flat_i = packRows(s.rows);
    dut.pivot_cols_flat_i = packColumns(s.cols);
    dut.row_gt1_i = s.rowGt1;
    dut.row_gt2_i = s.rowGt2;
    dut.row_gt3_i = s.rowGt3;
    dut.col_gt1_i = s.colGt1;
    dut.col_gt2_i = s.colGt2;
    dut.col_gt3_i = s.colGt3;
    std::uint8_t valid = 0;
    std::uint32_t pointers = 0;
    std::uint8_t descriptors = 0;
    const std::uint64_t differing = packDifferingAddresses(s);
    for (int i = 0; i < kHybridEntries; ++i) {
        if (s.hybrids[i].valid) valid |= static_cast<std::uint8_t>(1U << i);
        pointers |= static_cast<std::uint32_t>((s.hybrids[i].pointer & 7U) << (3 * i));
        if (s.hybrids[i].descriptorRowDiff) descriptors |= static_cast<std::uint8_t>(1U << i);
    }
    dut.hybrid_valid_i = valid;
    dut.hybrid_pointer_flat_i = pointers;
    dut.hybrid_descriptor_i = descriptors;
    dut.hybrid_differing_flat_i = differing;
    dut.conventional_overflow_i = s.conventionalOverflow;
    dut.eval();
}

Result observed(const Vrecam_phase3b_verification_top &dut)
{
    return {static_cast<std::uint16_t>(dut.shared_candidate_valid_o),
            static_cast<std::uint8_t>(dut.shared_pattern_id_o),
            static_cast<bool>(dut.shared_solution_valid_o),
            static_cast<bool>(dut.shared_repairable_o),
            static_cast<bool>(dut.shared_dictionary_overflow_o)};
}

std::string stateSummary(int cfg, const State &s)
{
    return "cfg=" + std::to_string(cfg) + " pivots=" + std::to_string(s.pivotValid) +
           " rowGt={" + std::to_string(s.rowGt1) + "," + std::to_string(s.rowGt2) +
           "," + std::to_string(s.rowGt3) + "} colGt={" + std::to_string(s.colGt1) +
           "," + std::to_string(s.colGt2) + "," + std::to_string(s.colGt3) + "}";
}

void checkGolden(Vrecam_phase3b_verification_top &dut, int cfg, const State &s,
                 const std::string &label)
{
    drive(dut, cfg, s);
    const Result got = observed(dut);
    const Result want = golden(cfg, s);
    if (got.candidates != want.candidates || got.patternId != want.patternId ||
        got.solutionValid != want.solutionValid || got.repairable != want.repairable ||
        got.dictionaryOverflow != want.dictionaryOverflow) {
        fail(label + " golden mismatch: " + stateSummary(cfg, s) +
             " got(cand=" + std::to_string(got.candidates) + ",pid=" +
             std::to_string(got.patternId) + ",sol=" + std::to_string(got.solutionValid) +
             ",rep=" + std::to_string(got.repairable) + ",dict=" +
             std::to_string(got.dictionaryOverflow) + ") want(cand=" +
             std::to_string(want.candidates) + ",pid=" + std::to_string(want.patternId) +
             ",sol=" + std::to_string(want.solutionValid) + ",rep=" +
             std::to_string(want.repairable) + ",dict=" +
             std::to_string(want.dictionaryOverflow) + ")");
    }
}

bool compactPrefix(std::uint8_t valid, int k)
{
    bool sawInvalid = false;
    for (int i = 0; i < k; ++i) {
        if (!bit(valid, i)) sawInvalid = true;
        else if (sawInvalid) return false;
    }
    return true;
}

State pivots(int count)
{
    State s;
    s.pivotValid = static_cast<std::uint8_t>((1U << count) - 1U);
    for (int i = 0; i < kMaxK; ++i) {
        s.rows[i] = static_cast<std::uint16_t>(10 + i);
        s.cols[i] = static_cast<std::uint16_t>(100 + i);
    }
    return s;
}

bool phase3aEquivalent(Vrecam_phase3b_verification_top &dut, const State &s,
                       std::string &diagnostic)
{
    drive(dut, 0, s);
    const auto sharedCandidates = static_cast<std::uint16_t>(dut.shared_candidate_valid_o & 0x3fU);
    if (sharedCandidates != dut.phase3a_candidate_valid_o ||
        dut.shared_pattern_id_o != dut.phase3a_pattern_id_o ||
        dut.shared_solution_valid_o != dut.phase3a_repairable_o ||
        dut.shared_repairable_o != dut.phase3a_repairable_o) {
        diagnostic = stateSummary(0, s) + " p3a(cand=" +
            std::to_string(dut.phase3a_candidate_valid_o) + ",pid=" +
            std::to_string(dut.phase3a_pattern_id_o) + ") p3b(cand=" +
            std::to_string(sharedCandidates) + ",pid=" +
            std::to_string(dut.shared_pattern_id_o) + ")";
        return false;
    }
    return true;
}

void checkPhase3aGolden(Vrecam_phase3b_verification_top &dut, const State &s,
                        const std::string &label)
{
    const Result want = golden(0, s);
    const std::uint16_t gotCandidates = dut.phase3a_candidate_valid_o;
    if (gotCandidates != want.candidates || dut.phase3a_pattern_id_o != want.patternId ||
        static_cast<bool>(dut.phase3a_repairable_o) != want.repairable) {
        fail(label + " Phase 3A golden mismatch: " + stateSummary(0, s) +
             " got(cand=" + std::to_string(gotCandidates) + ",pid=" +
             std::to_string(dut.phase3a_pattern_id_o) + ",rep=" +
             std::to_string(dut.phase3a_repairable_o) + ") want(cand=" +
             std::to_string(want.candidates) + ",pid=" +
             std::to_string(want.patternId) + ",rep=" +
             std::to_string(want.repairable) + ")");
    }
}

State transposeState(const State &in)
{
    State out = in;
    out.rows = in.cols;
    out.cols = in.rows;
    out.rowGt1 = in.colGt1; out.rowGt2 = in.colGt2; out.rowGt3 = in.colGt3;
    out.colGt1 = in.rowGt1; out.colGt2 = in.rowGt2; out.colGt3 = in.rowGt3;
    for (Hybrid &h : out.hybrids)
        if (h.valid) h.descriptorRowDiff = !h.descriptorRowDiff;
    return out;
}

void checkTransposePair(Vrecam_phase3b_verification_top &dut, int normalCfg, int transposedCfg)
{
    State normal = pivots(2);
    normal.rowGt1 = 1;
    normal.colGt2 = 2;
    normal.hybrids[0] = {true, 0, true, 77};
    normal.hybrids[1] = {true, 1, false, 88};
    const State transposed = transposeState(normal);
    checkGolden(dut, normalCfg, normal, "transpose normal");
    const Result a = observed(dut);
    checkGolden(dut, transposedCfg, transposed, "transpose physical");
    const Result b = observed(dut);
    require(a.candidates == b.candidates && a.patternId == b.patternId &&
            a.solutionValid == b.solutionValid && a.repairable == b.repairable,
            "transpose pair mismatch " + std::to_string(normalCfg) + "/" +
            std::to_string(transposedCfg));
}

} // namespace

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vrecam_phase3b_verification_top dut;
    std::size_t equivalenceVectors = 0;
    std::size_t equivalenceMismatches = 0;
    std::string firstEquivalenceMismatch;

    try {
        // The upstream CAM writes entry occupancy_o and therefore produces a
        // compact low-index prefix. This is a testbench contract check, not DUT hardware.
        require(compactPrefix(0b00111, 5), "compact-prefix positive check failed");
        require(!compactPrefix(0b00101, 5), "sparse-valid violation was not detected");
        std::cout << "PREFIX_VALID_CONTRACT PASS (sparse negative detected)\n";

        // Directed Phase 3A equivalence, including both descriptor polarities.
        std::vector<State> directed;
        directed.push_back(pivots(0));
        directed.push_back(pivots(1));
        State must = pivots(3); must.rowGt2 = 1; must.colGt2 = 2; directed.push_back(must);
        State rowExtension = pivots(2); rowExtension.hybrids[0] = {true, 0, true, 77}; directed.push_back(rowExtension);
        State colExtension = pivots(2); colExtension.hybrids[0] = {true, 1, false, 88}; directed.push_back(colExtension);
        State rowFull = pivots(4); rowFull.hybrids[0] = {true, 0, false, 31}; directed.push_back(rowFull);
        State colFull = pivots(4); colFull.hybrids[0] = {true, 0, true, 487}; directed.push_back(colFull);
        State overflow = pivots(2); overflow.conventionalOverflow = true; directed.push_back(overflow);
        for (std::size_t i = 0; i < directed.size(); ++i) {
            checkGolden(dut, 0, directed[i], "directed equivalence");
            checkPhase3aGolden(dut, directed[i], "directed equivalence");
            std::string diagnostic;
            if (!phase3aEquivalent(dut, directed[i], diagnostic)) {
                if (firstEquivalenceMismatch.empty())
                    firstEquivalenceMismatch = "directed vector " + std::to_string(i) + ": " + diagnostic;
                ++equivalenceMismatches;
            }
            ++equivalenceVectors;
        }

        std::mt19937 rng(0x3b202609U);
        for (int vector = 0; vector < 2000; ++vector) {
            const int count = static_cast<int>(rng() % 5U);
            State s = pivots(count);
            for (int i = 0; i < count; ++i) {
                const int rowFaults = 1 + static_cast<int>(rng() % 5U);
                const int colFaults = 1 + static_cast<int>(rng() % 5U);
                if (rowFaults > 1) s.rowGt1 |= static_cast<std::uint8_t>(1U << i);
                if (rowFaults > 2) s.rowGt2 |= static_cast<std::uint8_t>(1U << i);
                if (rowFaults > 3) s.rowGt3 |= static_cast<std::uint8_t>(1U << i);
                if (colFaults > 1) s.colGt1 |= static_cast<std::uint8_t>(1U << i);
                if (colFaults > 2) s.colGt2 |= static_cast<std::uint8_t>(1U << i);
                if (colFaults > 3) s.colGt3 |= static_cast<std::uint8_t>(1U << i);
            }
            const int hybridCount = count == 0 ? 0 : static_cast<int>(rng() % 5U);
            int rowExtra = 0, colExtra = 0;
            for (int h = 0; h < hybridCount; ++h) {
                const bool rowDiff = (rng() & 1U) != 0;
                const int pointer = static_cast<int>(rng() % static_cast<unsigned>(count));
                bool makeExtra = (rng() & 1U) != 0;
                if (rowDiff && count + rowExtra >= 4) makeExtra = false;
                if (!rowDiff && count + colExtra >= 4) makeExtra = false;
                std::uint16_t differing;
                if (makeExtra) {
                    differing = static_cast<std::uint16_t>((rowDiff ? 300 : 500) + h);
                    if (rowDiff) ++rowExtra; else ++colExtra;
                } else {
                    const int existing = static_cast<int>(rng() % static_cast<unsigned>(count));
                    differing = rowDiff ? s.rows[existing] : s.cols[existing];
                }
                s.hybrids[h] = {true, static_cast<std::uint8_t>(pointer), rowDiff, differing};
            }
            s.conventionalOverflow = (rng() % 31U) == 0;
            checkGolden(dut, 0, s, "random equivalence");
            checkPhase3aGolden(dut, s, "random equivalence");
            std::string diagnostic;
            if (!phase3aEquivalent(dut, s, diagnostic)) {
                if (firstEquivalenceMismatch.empty())
                    firstEquivalenceMismatch = "random vector " + std::to_string(vector) + ": " + diagnostic;
                ++equivalenceMismatches;
            }
            ++equivalenceVectors;
        }
        std::cout << "PHASE3A_CFG0_EQUIVALENCE "
                  << (equivalenceMismatches == 0 ? "PASS" : "FAIL")
                  << " vectors=" << equivalenceVectors
                  << " mismatches=" << equivalenceMismatches << "\n";
        if (equivalenceMismatches == 0)
            std::cout << "PHASE3A_GOLDEN PASS vectors=" << equivalenceVectors << "\n"
                      << "PHASE3B_CFG0_GOLDEN PASS vectors=" << equivalenceVectors << "\n";
        if (!firstEquivalenceMismatch.empty())
            std::cout << "FIRST_EQUIVALENCE_MISMATCH " << firstEquivalenceMismatch << "\n";

        // Four required categories for every physical ConfigID.
        for (int cfg = 0; cfg <= 6; ++cfg) {
            const Geometry g = geometry(cfg);
            State repairable = pivots(0);
            checkGolden(dut, cfg, repairable, "cfg repairable");
            require(observed(dut).repairable, "repairable case rejected for CFG" + std::to_string(cfg));

            State nonMust = pivots(1);
            checkGolden(dut, cfg, nonMust, "cfg non-Must");
            require(observed(dut).repairable, "non-Must case rejected for CFG" + std::to_string(cfg));

            State mustCase = pivots(1);
            if (!g.transpose) mustCase.rowGt1 = mustCase.rowGt2 = mustCase.rowGt3 = 1;
            else mustCase.colGt1 = mustCase.colGt2 = mustCase.colGt3 = 1;
            checkGolden(dut, cfg, mustCase, "cfg Must");
            require(observed(dut).candidates != golden(cfg, nonMust).candidates,
                    "Must case did not affect CFG" + std::to_string(cfg));

            State unrepairable = pivots(g.rows + g.cols);
            unrepairable.rowGt1 = unrepairable.rowGt2 = unrepairable.rowGt3 = 0x1f;
            unrepairable.colGt1 = unrepairable.colGt2 = unrepairable.colGt3 = 0x1f;
            checkGolden(dut, cfg, unrepairable, "cfg unrepairable");
            require(!observed(dut).repairable, "unrepairable case accepted for CFG" + std::to_string(cfg));
            std::cout << "CFG" << cfg << " DIRECTED PASS\n";
        }

        // Physical threshold selection, including thresholds used through transpose.
        struct ThresholdCase { int cfg; bool row; int threshold; const char *name; };
        const std::array<ThresholdCase, 6> thresholds{{
            {1, true, 1, "CS1/row_gt1"}, {0, true, 2, "CS2/row_gt2"},
            {5, true, 3, "CS3/row_gt3"}, {4, false, 1, "RS1/col_gt1"},
            {0, false, 2, "RS2/col_gt2"}, {2, false, 3, "RS3/col_gt3"}}};
        for (const auto &tc : thresholds) {
            State base = pivots(1);
            checkGolden(dut, tc.cfg, base, "threshold base");
            const Result baseResult = observed(dut);
            State selected = base;
            std::uint8_t *field = nullptr;
            if (tc.row) field = tc.threshold == 1 ? &selected.rowGt1 : tc.threshold == 2 ? &selected.rowGt2 : &selected.rowGt3;
            else field = tc.threshold == 1 ? &selected.colGt1 : tc.threshold == 2 ? &selected.colGt2 : &selected.colGt3;
            *field = 1;
            checkGolden(dut, tc.cfg, selected, tc.name);
            require(observed(dut).candidates != baseResult.candidates,
                    std::string(tc.name) + " did not alter candidate validity");
        }
        State sameMetadata = pivots(1); sameMetadata.rowGt1 = 1;
        checkGolden(dut, 1, sameMetadata, "threshold differing CFG1"); const Result cfg1Threshold = observed(dut);
        checkGolden(dut, 0, sameMetadata, "threshold differing CFG0"); const Result cfg0Threshold = observed(dut);
        require(cfg1Threshold.candidates != cfg0Threshold.candidates,
                "same metadata across different ConfigIDs did not change candidate validity");
        std::cout << "THRESHOLD_DECODE PASS\n";

        // MAX_K=5 and pointer 4 must add a real off-diagonal constraint.
        State pointerFour = pivots(5);
        pointerFour.hybrids[0] = {true, 4, true, pointerFour.rows[0]};
        checkGolden(dut, 2, pointerFour, "pointer4");
        const Result pointerFourResult = observed(dut);
        State pointerZero = pointerFour;
        pointerZero.hybrids[0].pointer = 0;
        checkGolden(dut, 2, pointerZero, "pointer0 control");
        require(pointerFourResult.candidates != observed(dut).candidates,
                "pivot[4]/pointer=4 did not affect candidate validity");
        std::cout << "MAX_K5_PIVOT4 PASS\n";

        // Exercise non-overlapping high bits in the frozen 9-bit row and
        // 5-bit ColumnWord fields.  Each Hybrid must match the intended
        // dictionary entry rather than a truncated low-bit alias.
        State highRow = pivots(2);
        highRow.rows[0] = 0x080; highRow.rows[1] = 0x180;
        highRow.hybrids[0] = {true, 0, true, highRow.rows[1]};
        State highRowControl = highRow;
        highRowControl.hybrids[0].valid = false;
        checkGolden(dut, 0, highRowControl, "high-row control");
        const Result highRowBase = observed(dut);
        checkGolden(dut, 0, highRow, "high-row MSB");
        checkPhase3aGolden(dut, highRow, "high-row MSB");
        std::string highRowDiagnostic;
        require(phase3aEquivalent(dut, highRow, highRowDiagnostic), highRowDiagnostic);
        require(observed(dut).candidates != highRowBase.candidates,
                "9-bit row MSB did not affect dictionary matching");
        std::cout << "ROW_ADDR_W9_HIGH_BIT PASS\n";

        State highColumn = pivots(2);
        highColumn.cols[0] = 0x01; highColumn.cols[1] = 0x11;
        highColumn.hybrids[0] = {true, 0, false, highColumn.cols[1]};
        State highColumnControl = highColumn;
        highColumnControl.hybrids[0].valid = false;
        checkGolden(dut, 0, highColumnControl, "high-column control");
        const Result highColumnBase = observed(dut);
        checkGolden(dut, 0, highColumn, "high-column MSB");
        checkPhase3aGolden(dut, highColumn, "high-column MSB");
        std::string highColumnDiagnostic;
        require(phase3aEquivalent(dut, highColumn, highColumnDiagnostic), highColumnDiagnostic);
        require(observed(dut).candidates != highColumnBase.candidates,
                "5-bit ColumnWord MSB did not affect dictionary matching");
        std::cout << "COL_ADDR_W5_HIGH_BIT PASS\n";

        State zeroExtension = pivots(2);
        zeroExtension.cols[0] = 0x01; zeroExtension.cols[1] = 0x1f;
        zeroExtension.hybrids[0] = {true, 0, false, 0x1f};
        require((packDifferingAddresses(zeroExtension) & kDiffAddrMask) == 0x01f,
                "ColumnWord differing address was not zero-extended to 9 bits");
        checkGolden(dut, 0, zeroExtension, "column zero-extension");
        checkPhase3aGolden(dut, zeroExtension, "column zero-extension");
        std::string zeroExtensionDiagnostic;
        require(phase3aEquivalent(dut, zeroExtension, zeroExtensionDiagnostic), zeroExtensionDiagnostic);
        std::cout << "COLUMNWORD_ZERO_EXTENSION_5_TO_9 PASS\n";
        std::cout << "ASYMMETRIC_ADDRESS_WIDTH PASS\n";

        // CFG2 uses MAX_K=5.  The seventh physical Hybrid entry must enter
        // the matrix and change the candidate bitmap, not merely toggle a port.
        State hybridSix = pivots(5);
        State hybridSixControl = hybridSix;
        checkGolden(dut, 2, hybridSixControl, "Hybrid entry6 control");
        const Result hybridSixBase = observed(dut);
        hybridSix.hybrids[6] = {true, 0, false, hybridSix.cols[1]};
        checkGolden(dut, 2, hybridSix, "Hybrid entry6");
        require(observed(dut).candidates != hybridSixBase.candidates,
                "Hybrid entry[6] did not affect CFG2 candidate validity");
        std::cout << "MAX_HYBRID_ENTRIES7_ENTRY6 PASS\n";

        checkTransposePair(dut, 1, 4); std::cout << "TRANSPOSE 2R1C_1R2C PASS\n";
        checkTransposePair(dut, 2, 5); std::cout << "TRANSPOSE 3R2C_2R3C PASS\n";
        checkTransposePair(dut, 3, 6); std::cout << "TRANSPOSE 3R1C_1R3C PASS\n";

        State invalidCfg = pivots(1);
        checkGolden(dut, 7, invalidCfg, "CFG7");
        require(observed(dut).candidates == 0 && !observed(dut).repairable, "CFG7 was not rejected");
        State conventional = pivots(1); conventional.conventionalOverflow = true;
        checkGolden(dut, 0, conventional, "conventional overflow");
        require(observed(dut).candidates == 0 && !observed(dut).repairable,
                "conventional overflow was not rejected");
        State rowFullExpansion = pivots(4); rowFullExpansion.hybrids[0] = {true, 0, false, 31};
        checkGolden(dut, 0, rowFullExpansion, "row dictionary full");
        require(!observed(dut).dictionaryOverflow && observed(dut).candidates == 0x07 &&
                observed(dut).solutionValid && observed(dut).repairable,
                "row dictionary full must expand to a required-row constraint");
        State colFullExpansion = pivots(4); colFullExpansion.hybrids[0] = {true, 0, true, 487};
        checkGolden(dut, 0, colFullExpansion, "column dictionary full");
        require(!observed(dut).dictionaryOverflow && observed(dut).candidates == 0x38 &&
                observed(dut).solutionValid && observed(dut).repairable,
                "column dictionary full must expand to a required-column constraint");
        State badPointer = pivots(2); badPointer.hybrids[0] = {true, 4, true, 99};
        checkGolden(dut, 0, badPointer, "invalid Hybrid pointer");
        require(observed(dut).dictionaryOverflow, "invalid Hybrid pointer was not rejected");
        State noCandidate = pivots(4);
        noCandidate.rowGt2 = noCandidate.colGt2 = 0x0f;
        checkGolden(dut, 0, noCandidate, "no candidate");
        require(observed(dut).candidates == 0 && !observed(dut).solutionValid &&
                !observed(dut).repairable, "no-valid-candidate case was not rejected");
        std::cout << "OVERFLOW_INVALID_NEGATIVE PASS\n";

        if (equivalenceMismatches != 0) {
            std::cerr << "PHASE3B_FUNCTIONAL_REGRESSION FAIL: Phase 3A and Phase 3B CFG0 differ\n";
            dut.final();
            return 1;
        }
        std::cout << "PHASE3B_FUNCTIONAL_REGRESSION PASS\n";
    } catch (const std::exception &error) {
        std::cerr << "PHASE3B_FUNCTIONAL_REGRESSION FAIL: " << error.what() << '\n';
        dut.final();
        return 1;
    }
    dut.final();
    return 0;
}
