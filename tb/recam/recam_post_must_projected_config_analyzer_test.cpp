#include "Vtb_recam_post_must_projected_config_analyzer.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

double sc_time_stamp()
{
    return 0.0;
}

namespace {
constexpr int kMaxK = 5;
constexpr int kConfigs = 7;
constexpr int kPhysicalEntries = 14;
constexpr int kProjectedEntries = 9;

struct Geometry {
    int rows;
    int cols;
    bool transpose;
};

const std::array<Geometry, kConfigs> kGeometry{{
    {2, 2, false}, {2, 1, false}, {3, 2, false}, {3, 1, false},
    {2, 1, true}, {3, 2, true}, {3, 1, true},
}};

const std::array<Geometry, kConfigs> kPhysicalGeometry{{
    {2, 2, false}, {2, 1, false}, {3, 2, false}, {3, 1, false},
    {1, 2, false}, {2, 3, false}, {1, 3, false},
}};

struct PhysicalHybrid {
    bool valid = false;
    std::uint8_t pointer = 0;
    bool descriptor_column = false;
    std::uint16_t differing = 0;
    std::uint8_t cfg_valid = 0;
};

struct LogicalHybrid {
    std::uint8_t pointer = 0;
    bool descriptor_column = false;
    std::uint16_t differing = 0;
};

struct State {
    std::array<std::uint16_t, kMaxK> pivot_rows{};
    std::array<std::uint16_t, kMaxK> pivot_cols{};
    std::array<int, kMaxK> row_counts{};
    std::array<int, kMaxK> col_counts{};
    int pivot_count = 0;
    std::array<PhysicalHybrid, kPhysicalEntries> physical{};
    bool conventional_overflow = false;
};

struct Result {
    std::uint16_t candidates = 0;
    std::uint8_t pattern = 0;
    bool solution = false;
    bool repairable = false;
    bool dictionary_overflow = false;
};

struct Statistics {
    int states = 0;
    int config_evaluations = 0;
    int membership_gt9 = 0;
    int post_must_eq9 = 0;
    int post_must_gt9 = 0;
    int backward_le7 = 0;
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

int configK(int config)
{
    return kGeometry.at(config).rows + kGeometry.at(config).cols;
}

bool bit(std::uint16_t value, int index)
{
    return ((value >> index) & 1U) != 0;
}

std::uint8_t membershipMask(int pointer)
{
    std::uint8_t mask = 0;
    for (int config = 0; config < kConfigs; ++config)
        if (pointer < configK(config))
            mask |= static_cast<std::uint8_t>(1U << config);
    return mask;
}

bool finalMustRetired(const State &state, int config, const PhysicalHybrid &hybrid)
{
    const Geometry &geometry = kPhysicalGeometry.at(config);
    if (hybrid.pointer >= kMaxK)
        return false;
    return hybrid.descriptor_column
        ? state.col_counts[hybrid.pointer] > geometry.rows
        : state.row_counts[hybrid.pointer] > geometry.cols;
}

std::vector<LogicalHybrid> postMustView(const State &state, int config, int *membership_count)
{
    std::vector<LogicalHybrid> view;
    *membership_count = 0;
    for (const PhysicalHybrid &hybrid : state.physical) {
        const bool member = hybrid.valid && ((hybrid.cfg_valid >> config) & 1U) != 0;
        if (member)
            ++*membership_count;
        if (member && hybrid.pointer < kMaxK && !finalMustRetired(state, config, hybrid))
            view.push_back({hybrid.pointer, hybrid.descriptor_column, hybrid.differing});
    }
    return view;
}

bool threshold(const State &state, bool row_dimension, int value, int index)
{
    const int count = row_dimension ? state.row_counts[index] : state.col_counts[index];
    return count > value;
}

std::vector<std::uint8_t> patterns(const Geometry &geometry)
{
    if (geometry.rows == 2 && geometry.cols == 1)
        return {4, 2, 1};
    if (geometry.rows == 2 && geometry.cols == 2)
        return {12, 10, 6, 9, 5, 3};
    if (geometry.rows == 3 && geometry.cols == 1)
        return {8, 4, 2, 1};
    return {24, 20, 12, 18, 10, 6, 17, 9, 5, 3};
}

Result historicalAnalyzer(const State &state, int config, const std::vector<LogicalHybrid> &hybrids)
{
    const Geometry &geometry = kGeometry.at(config);
    const int matrix_size = geometry.rows + geometry.cols;
    std::array<std::array<bool, kMaxK>, kMaxK> matrix{};
    std::array<std::uint16_t, kMaxK> row_dictionary{};
    std::array<std::uint16_t, kMaxK> col_dictionary{};
    Result output;
    int pivot_count = 0;
    for (int pivot = 0; pivot < kMaxK; ++pivot) {
        if (pivot < matrix_size && pivot < state.pivot_count) {
            row_dictionary[pivot] = geometry.transpose ? state.pivot_cols[pivot] : state.pivot_rows[pivot];
            col_dictionary[pivot] = geometry.transpose ? state.pivot_rows[pivot] : state.pivot_cols[pivot];
            ++pivot_count;
        }
    }
    int row_count = pivot_count;
    int col_count = pivot_count;
    for (int row = 0; row < matrix_size; ++row) {
        for (int col = 0; col < matrix_size; ++col) {
            const bool row_must = row < pivot_count && (geometry.cols == 1
                ? threshold(state, geometry.transpose ? false : true, 1, row)
                : geometry.cols == 2
                    ? threshold(state, geometry.transpose ? false : true, 2, row)
                    : threshold(state, geometry.transpose ? false : true, 3, row));
            const bool col_must = col < pivot_count && (geometry.rows == 1
                ? threshold(state, geometry.transpose ? true : false, 1, col)
                : geometry.rows == 2
                    ? threshold(state, geometry.transpose ? true : false, 2, col)
                    : threshold(state, geometry.transpose ? true : false, 3, col));
            matrix[row][col] = (row == col && row < pivot_count) || row_must || col_must;
        }
    }
    for (const LogicalHybrid &hybrid : hybrids) {
        if (hybrid.pointer >= pivot_count) {
            output.dictionary_overflow = true;
            continue;
        }
        const bool differing_is_row = hybrid.descriptor_column != geometry.transpose;
        std::array<std::uint16_t, kMaxK> &dictionary = differing_is_row ? row_dictionary : col_dictionary;
        int &dictionary_count = differing_is_row ? row_count : col_count;
        int differing_index = 0;
        bool differing_match = false;
        for (int index = 0; index < dictionary_count && !differing_match; ++index) {
            if (dictionary[index] == hybrid.differing) {
                differing_match = true;
                differing_index = index;
            }
        }
        if (!differing_match && dictionary_count < matrix_size) {
            differing_index = dictionary_count;
            dictionary[dictionary_count++] = hybrid.differing;
            differing_match = true;
        }
        if (differing_match) {
            if (differing_is_row)
                matrix[differing_index][hybrid.pointer] = true;
            else
                matrix[hybrid.pointer][differing_index] = true;
        } else if (differing_is_row) {
            for (int row = 0; row < row_count; ++row)
                matrix[row][hybrid.pointer] = true;
        } else {
            for (int col = 0; col < col_count; ++col)
                matrix[hybrid.pointer][col] = true;
        }
    }
    if (state.conventional_overflow || output.dictionary_overflow)
        return output;
    const std::vector<std::uint8_t> candidate_patterns = patterns(geometry);
    for (std::size_t candidate = 0; candidate < candidate_patterns.size(); ++candidate) {
        bool candidate_valid = true;
        for (int row = 0; row < matrix_size; ++row) {
            for (int col = 0; col < matrix_size; ++col) {
                if (matrix[row][col] && bit(candidate_patterns[candidate], row) &&
                    !bit(candidate_patterns[candidate], col))
                    candidate_valid = false;
            }
        }
        if (candidate_valid) {
            output.candidates |= static_cast<std::uint16_t>(1U << candidate);
            if (!output.solution) {
                output.solution = true;
                output.repairable = true;
                output.pattern = static_cast<std::uint8_t>(candidate + 1);
            }
        }
    }
    return output;
}

State collect(const std::vector<std::pair<std::uint16_t, std::uint16_t>> &faults)
{
    require(faults.size() <= 12, "collector trace exceeds the frozen fault budget");
    State state;
    std::vector<std::pair<std::uint16_t, std::uint16_t>> pivots;
    std::map<std::uint16_t, int> row_counts;
    std::map<std::uint16_t, int> col_counts;
    int physical_index = 0;
    for (const auto &fault : faults) {
        int pointer = -1;
        bool descriptor_column = false;
        for (std::size_t pivot = 0; pivot < pivots.size(); ++pivot) {
            if (fault.first == pivots[pivot].first) {
                pointer = static_cast<int>(pivot);
                break;
            }
            if (fault.second == pivots[pivot].second) {
                pointer = static_cast<int>(pivot);
                descriptor_column = true;
                break;
            }
        }
        ++row_counts[fault.first];
        ++col_counts[fault.second];
        if (pointer < 0) {
            if (pivots.size() < kMaxK)
                pivots.push_back(fault);
        } else if (physical_index < kPhysicalEntries) {
            PhysicalHybrid &hybrid = state.physical[physical_index++];
            hybrid.valid = true;
            hybrid.pointer = static_cast<std::uint8_t>(pointer);
            hybrid.descriptor_column = descriptor_column;
            hybrid.differing = descriptor_column ? fault.first : fault.second;
            hybrid.cfg_valid = membershipMask(pointer);
        }
    }
    state.pivot_count = static_cast<int>(pivots.size());
    for (int pivot = 0; pivot < state.pivot_count; ++pivot) {
        state.pivot_rows[pivot] = pivots[pivot].first;
        state.pivot_cols[pivot] = pivots[pivot].second;
        state.row_counts[pivot] = row_counts[pivots[pivot].first];
        state.col_counts[pivot] = col_counts[pivots[pivot].second];
    }
    return state;
}

State starState(int pivot_count, const std::vector<int> &row_extras,
                const std::vector<int> &col_extras)
{
    std::vector<std::pair<std::uint16_t, std::uint16_t>> faults;
    for (int pivot = 0; pivot < pivot_count; ++pivot)
        faults.push_back({static_cast<std::uint16_t>(100 + pivot), static_cast<std::uint16_t>(200 + pivot)});
    std::uint16_t next = 500;
    for (int pivot = 0; pivot < pivot_count; ++pivot) {
        for (int count = 0; count < row_extras[pivot]; ++count)
            faults.push_back({static_cast<std::uint16_t>(100 + pivot), next++});
        for (int count = 0; count < col_extras[pivot]; ++count)
            faults.push_back({next++, static_cast<std::uint16_t>(200 + pivot)});
    }
    return collect(faults);
}

State holeyState()
{
    State state;
    state.pivot_count = 5;
    for (int pivot = 0; pivot < kMaxK; ++pivot) {
        state.pivot_rows[pivot] = static_cast<std::uint16_t>(10 + pivot);
        state.pivot_cols[pivot] = static_cast<std::uint16_t>(20 + pivot);
        state.row_counts[pivot] = 1;
        state.col_counts[pivot] = 1;
    }
    state.row_counts[2] = 3;
    state.col_counts[4] = 3;
    state.physical[0] = {true, 0, false, 100, 0x01};
    state.physical[1] = {true, 1, false, 101, 0x00};
    state.physical[2] = {true, 2, false, 102, 0x01};
    state.physical[3] = {true, 3, true,  103, 0x01};
    state.physical[4] = {false, 0, false, 0, 0x01};
    state.physical[5] = {true, 4, true,  104, 0x01};
    state.physical[6] = {true, 0, false, 105, 0x01};
    return state;
}

void setWideBit(WData *words, int index, bool value)
{
    const int word = index / 32;
    const int bit_index = index % 32;
    if (value)
        words[word] |= static_cast<WData>(1U << bit_index);
}

void setWideField(WData *words, int start, int width, std::uint16_t value)
{
    for (int bit_index = 0; bit_index < width; ++bit_index)
        setWideBit(words, start + bit_index, ((value >> bit_index) & 1U) != 0);
}

std::uint16_t getWideField(const WData *words, int start, int width)
{
    std::uint16_t value = 0;
    for (int bit_index = 0; bit_index < width; ++bit_index) {
        const int source = start + bit_index;
        if ((words[source / 32] >> (source % 32)) & 1U)
            value |= static_cast<std::uint16_t>(1U << bit_index);
    }
    return value;
}

void apply(Vtb_recam_post_must_projected_config_analyzer &dut, const State &state, int config)
{
    dut.config_id_i = static_cast<std::uint8_t>(config);
    dut.pivot_valid_i = state.pivot_count == 0 ? 0 : static_cast<std::uint8_t>((1U << state.pivot_count) - 1U);
    dut.pivot_rows_flat_i = 0;
    dut.pivot_cols_flat_i = 0;
    dut.row_gt1_i = 0;
    dut.row_gt2_i = 0;
    dut.row_gt3_i = 0;
    dut.col_gt1_i = 0;
    dut.col_gt2_i = 0;
    dut.col_gt3_i = 0;
    for (int pivot = 0; pivot < state.pivot_count; ++pivot) {
        dut.pivot_rows_flat_i |= static_cast<std::uint64_t>(state.pivot_rows[pivot]) << (pivot * 9);
        dut.pivot_cols_flat_i |= static_cast<std::uint32_t>(state.pivot_cols[pivot]) << (pivot * 5);
        if (state.row_counts[pivot] > 1) dut.row_gt1_i |= static_cast<std::uint8_t>(1U << pivot);
        if (state.row_counts[pivot] > 2) dut.row_gt2_i |= static_cast<std::uint8_t>(1U << pivot);
        if (state.row_counts[pivot] > 3) dut.row_gt3_i |= static_cast<std::uint8_t>(1U << pivot);
        if (state.col_counts[pivot] > 1) dut.col_gt1_i |= static_cast<std::uint8_t>(1U << pivot);
        if (state.col_counts[pivot] > 2) dut.col_gt2_i |= static_cast<std::uint8_t>(1U << pivot);
        if (state.col_counts[pivot] > 3) dut.col_gt3_i |= static_cast<std::uint8_t>(1U << pivot);
    }
    dut.physical_hybrid_valid_i = 0;
    dut.physical_hybrid_pointer_flat_i = 0;
    dut.physical_hybrid_descriptor_i = 0;
    std::memset(&dut.physical_hybrid_differing_flat_i, 0, sizeof(dut.physical_hybrid_differing_flat_i));
    std::memset(&dut.physical_hybrid_cfg_valid_flat_i, 0, sizeof(dut.physical_hybrid_cfg_valid_flat_i));
    dut.row_must_by_cfg_i = 0;
    dut.col_must_by_cfg_i = 0;
    for (int physical = 0; physical < kPhysicalEntries; ++physical) {
        const PhysicalHybrid &hybrid = state.physical[physical];
        if (hybrid.valid)
            dut.physical_hybrid_valid_i |= static_cast<std::uint16_t>(1U << physical);
        dut.physical_hybrid_pointer_flat_i |= static_cast<std::uint64_t>(hybrid.pointer) << (physical * 3);
        if (hybrid.descriptor_column)
            dut.physical_hybrid_descriptor_i |= static_cast<std::uint16_t>(1U << physical);
        setWideField(dut.physical_hybrid_differing_flat_i, physical * 9, 9, hybrid.differing);
        for (int cfg = 0; cfg < kConfigs; ++cfg)
            if ((hybrid.cfg_valid >> cfg) & 1U)
                setWideBit(dut.physical_hybrid_cfg_valid_flat_i, physical * kConfigs + cfg, true);
    }
    for (int cfg = 0; cfg < kConfigs; ++cfg) {
        for (int pivot = 0; pivot < kMaxK; ++pivot) {
            if (pivot < state.pivot_count) {
                const Geometry &geometry = kPhysicalGeometry[cfg];
                if (state.row_counts[pivot] > geometry.cols)
                    dut.row_must_by_cfg_i |= static_cast<std::uint64_t>(1ULL << (cfg * kMaxK + pivot));
                if (state.col_counts[pivot] > geometry.rows)
                    dut.col_must_by_cfg_i |= static_cast<std::uint64_t>(1ULL << (cfg * kMaxK + pivot));
            }
        }
    }
    dut.conventional_overflow_i = state.conventional_overflow;
}

Result observedNew(const Vtb_recam_post_must_projected_config_analyzer &dut)
{
    return {static_cast<std::uint16_t>(dut.new_candidate_valid_o),
            static_cast<std::uint8_t>(dut.new_pattern_id_o),
            static_cast<bool>(dut.new_solution_valid_o), static_cast<bool>(dut.new_repairable_o),
            static_cast<bool>(dut.new_dictionary_overflow_o)};
}

Result observedOld(const Vtb_recam_post_must_projected_config_analyzer &dut)
{
    return {static_cast<std::uint16_t>(dut.old_candidate_valid_o),
            static_cast<std::uint8_t>(dut.old_pattern_id_o),
            static_cast<bool>(dut.old_solution_valid_o), static_cast<bool>(dut.old_repairable_o),
            static_cast<bool>(dut.old_dictionary_overflow_o)};
}

void compareResult(const Result &actual, const Result &expected, const std::string &label)
{
    require(actual.candidates == expected.candidates, label + ": candidate-valid mismatch");
    require(actual.pattern == expected.pattern, label + ": PatternID mismatch");
    require(actual.solution == expected.solution, label + ": solution-valid mismatch");
    require(actual.repairable == expected.repairable, label + ": repairable mismatch");
    require(actual.dictionary_overflow == expected.dictionary_overflow, label + ": dictionary-overflow mismatch");
}

void verifyState(Vtb_recam_post_must_projected_config_analyzer &dut, const State &state,
                 int config, Statistics &statistics, const std::string &label)
{
    int membership_count = 0;
    const std::vector<LogicalHybrid> view = postMustView(state, config, &membership_count);
    require(view.size() <= kProjectedEntries, label + ": legal post-Must view exceeds nine entries");
    apply(dut, state, config);
    dut.eval();
    require(!dut.projection_overflow_o, label + ": projector asserted overflow");
    require(dut.projected_count_o == view.size(), label + ": projected count mismatch");
    for (int slot = 0; slot < kProjectedEntries; ++slot) {
        const bool valid = slot < static_cast<int>(view.size());
        require(static_cast<bool>((dut.projected_hybrid_valid_o >> slot) & 1U) == valid,
                label + ": projected valid packing mismatch");
        if (valid) {
            require(((dut.projected_hybrid_pointer_flat_o >> (slot * 3)) & 7U) == view[slot].pointer,
                    label + ": projected pointer order mismatch");
            require(static_cast<bool>((dut.projected_hybrid_descriptor_o >> slot) & 1U) ==
                        view[slot].descriptor_column,
                    label + ": projected descriptor order mismatch");
            require(getWideField(dut.projected_hybrid_differing_flat_o, slot * 9, 9) == view[slot].differing,
                    label + ": projected differing-address order mismatch");
        }
    }
    compareResult(observedNew(dut), historicalAnalyzer(state, config, view), label + " historical equivalence");
    if (view.size() <= 7) {
        compareResult(observedOld(dut), observedNew(dut), label + " old <=7 equivalence");
        ++statistics.backward_le7;
    }
    ++statistics.config_evaluations;
    if (membership_count > 9) ++statistics.membership_gt9;
    if (view.size() == 9) ++statistics.post_must_eq9;
    if (view.size() > 9) ++statistics.post_must_gt9;
}

void verifyAllConfigs(Vtb_recam_post_must_projected_config_analyzer &dut, const State &state,
                      Statistics &statistics, const std::string &label)
{
    ++statistics.states;
    for (int config = 0; config < kConfigs; ++config)
        verifyState(dut, state, config, statistics, label + " cfg=" + std::to_string(config));
}

void requireViewCounts(const State &state, int config, int expected_membership,
                       int expected_post_must, const std::string &label)
{
    int membership_count = 0;
    const std::vector<LogicalHybrid> view = postMustView(state, config, &membership_count);
    require(membership_count == expected_membership,
            label + ": membership count mismatch actual=" + std::to_string(membership_count));
    require(static_cast<int>(view.size()) == expected_post_must,
            label + ": post-Must count mismatch actual=" + std::to_string(view.size()));
}

State randomReachableState(std::mt19937 &random)
{
    std::uniform_int_distribution<int> fault_count_distribution(1, 12);
    std::uniform_int_distribution<int> address_distribution(0, 31);
    std::vector<std::pair<std::uint16_t, std::uint16_t>> faults;
    std::vector<std::pair<std::uint16_t, std::uint16_t>> pivots;
    const int fault_count = fault_count_distribution(random);
    for (int fault = 0; fault < fault_count; ++fault) {
        std::uint16_t row = static_cast<std::uint16_t>(address_distribution(random));
        std::uint16_t col = static_cast<std::uint16_t>(address_distribution(random));
        if (!pivots.empty() && (random() % 100) < 70) {
            const auto &pivot = pivots[random() % pivots.size()];
            if ((random() & 1U) == 0)
                row = pivot.first;
            else
                col = pivot.second;
        }
        bool related = false;
        for (const auto &pivot : pivots)
            related = related || row == pivot.first || col == pivot.second;
        if (!related && pivots.size() < kMaxK)
            pivots.push_back({row, col});
        faults.push_back({row, col});
    }
    return collect(faults);
}
}  // namespace

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    try {
        Vtb_recam_post_must_projected_config_analyzer dut;
        Statistics statistics;

        const State row_blocker = collect({{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5},
                                           {0, 6}, {0, 7}, {0, 8}, {0, 9}, {0, 10}, {0, 11}});
        const State column_blocker = collect({{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0},
                                              {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}, {11, 0}});
        for (int config = 0; config < kConfigs; ++config) {
            requireViewCounts(row_blocker, config, 11, 0, "DR1 row blocker");
            requireViewCounts(column_blocker, config, 11, 0, "DR2 column blocker");
        }
        verifyAllConfigs(dut, row_blocker,
                         statistics, "DR1 row blocker");
        verifyAllConfigs(dut, column_blocker,
                         statistics, "DR2 column blocker");
        verifyAllConfigs(dut, collect({{0, 0}, {0, 1}, {1, 0}}), statistics, "DR3 mixed Must");
        verifyAllConfigs(dut, collect({{10, 20}, {11, 21}, {12, 22}, {13, 23}, {13, 100}, {13, 101}}),
                         statistics, "DR4 membership holes");
        const State holey = holeyState();
        requireViewCounts(holey, 0, 5, 3, "DR4 injected exact H-order");
        verifyAllConfigs(dut, holey, statistics, "DR4 injected exact H-order");
        const State config2_nine = starState(3, {1, 1, 1}, {2, 2, 2});
        const State config5_nine = starState(3, {2, 2, 2}, {1, 1, 1});
        requireViewCounts(config2_nine, 2, 9, 9, "DR5 config2 nine");
        requireViewCounts(config5_nine, 5, 9, 9, "DR6 config5 nine");
        verifyAllConfigs(dut, config2_nine, statistics, "DR5 config2 nine");
        verifyAllConfigs(dut, config5_nine, statistics, "DR6 config5 nine");

        std::mt19937 random(0xA2EA2U);
        for (int state = 0; state < 10000; ++state)
            verifyAllConfigs(dut, randomReachableState(random), statistics,
                             "random state=" + std::to_string(state));

        require(statistics.post_must_gt9 == 0, "reachable random campaign exceeded post-Must capacity");
        require(statistics.membership_gt9 > 0, "random campaign missed membership-visible >9 coverage");
        std::cout << "S1GA2EA2 RTL_EQUIVALENCE PASS states=" << statistics.states
                  << " config_evaluations=" << statistics.config_evaluations
                  << " membership_gt9=" << statistics.membership_gt9
                  << " post_must_eq9=" << statistics.post_must_eq9
                  << " post_must_gt9=" << statistics.post_must_gt9
                  << " backward_le7=" << statistics.backward_le7 << "\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "S1GA2EA2 RTL_EQUIVALENCE FAIL: " << error.what() << "\n";
        return 1;
    }
}
