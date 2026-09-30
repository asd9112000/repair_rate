#include "n3_rc_analyzer_oracle.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace n3_rc_analyzer_oracle {

uint32_t next_random(uint32_t& state) {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

std::string fixed_mask_key(unsigned rows, unsigned cols, unsigned pattern) {
    return std::to_string(rows) + ":" + std::to_string(cols) + ":" + std::to_string(pattern);
}

FixedMaskMap load_fixed_masks(const std::string& csv_path) {
    std::ifstream input(csv_path);
    if (!input) throw std::runtime_error("cannot open fixed-mask CSV: " + csv_path);
    std::string line;
    FixedMaskMap masks;
    std::getline(input, line);
    while (std::getline(input, line)) {
        std::stringstream row(line);
        std::array<std::string, 10> fields{};
        for (std::string& field : fields) std::getline(row, field, ',');
        masks.emplace(fixed_mask_key(std::stoul(fields[1]), std::stoul(fields[2]),
                                     std::stoul(fields[4])),
                      static_cast<uint8_t>(std::stoul(fields[7], nullptr, 2)));
    }
    if (masks.size() != 80U) throw std::runtime_error("fixed-mask CSV must contain 80 unique rows");
    return masks;
}

unsigned candidate_count(unsigned rows, unsigned cols) {
    if (rows == 3U && cols == 3U) return 20U;
    if (rows == 3U && cols == 2U) return 10U;
    if (rows == 4U && cols == 3U) return 35U;
    if (rows == 4U && cols == 2U) return 15U;
    return 0U;
}

namespace {
bool threshold(const std::array<std::array<bool, kMaxPivots>, kThresholdLevels>& values,
               unsigned count, unsigned index) {
    return count >= 1U && count <= kThresholdLevels ? values[count - 1U][index] : false;
}
}  // namespace

AnalyzerResult evaluate(const AnalyzerState& state, const CapacityRequest& request,
                        const FixedMaskMap& masks) {
    const unsigned rows = request.transpose ? request.cols : request.rows;
    const unsigned cols = request.transpose ? request.rows : request.cols;
    const unsigned matrix_size = rows + cols;
    std::array<uint16_t, kMaxPivots> row_dict{};
    std::array<uint16_t, kMaxPivots> col_dict{};
    std::array<std::array<bool, kMaxPivots>, kMaxPivots> matrix{};
    unsigned pivots = 0U;
    for (unsigned pivot = 0; pivot < kMaxPivots; ++pivot) {
        if (pivot < matrix_size && state.pivot_valid[pivot]) {
            row_dict[pivot] = request.transpose ? state.pivot_cols[pivot] : state.pivot_rows[pivot];
            col_dict[pivot] = request.transpose ? state.pivot_rows[pivot] : state.pivot_cols[pivot];
            ++pivots;
        }
    }
    unsigned row_count = pivots;
    unsigned col_count = pivots;
    for (unsigned row = 0; row < kMaxPivots; ++row) {
        for (unsigned col = 0; col < kMaxPivots; ++col) {
            if (row < matrix_size && col < matrix_size) {
                const bool diagonal = row == col && row < pivots;
                const bool row_relation = row < pivots &&
                    (request.transpose ? threshold(state.col_gt, cols, row)
                                       : threshold(state.row_gt, cols, row));
                const bool col_relation = col < pivots &&
                    (request.transpose ? threshold(state.row_gt, rows, col)
                                       : threshold(state.col_gt, rows, col));
                matrix[row][col] = diagonal || row_relation || col_relation;
            }
        }
    }
    bool dictionary_overflow = false;
    for (unsigned hybrid = 0; hybrid < kHybridEntries; ++hybrid) {
        if (!state.hybrid_valid[hybrid]) continue;
        const unsigned pointer = state.hybrid_pointer[hybrid];
        const bool differing_is_row = state.hybrid_descriptor[hybrid] ^ request.transpose;
        const uint16_t differing = state.hybrid_differing[hybrid];
        if (pointer >= pivots) { dictionary_overflow = true; continue; }
        bool found = false;
        unsigned differing_index = 0U;
        if (differing_is_row) {
            for (unsigned index = 0; index < kMaxPivots; ++index)
                if (!found && index < row_count && differing == row_dict[index]) {
                    found = true; differing_index = index;
                }
            if (!found && row_count < matrix_size) {
                differing_index = row_count; row_dict[row_count++] = differing; found = true;
            }
            if (found) matrix[differing_index][pointer] = true;
            else for (unsigned index = 0; index < row_count; ++index) matrix[index][pointer] = true;
        } else {
            for (unsigned index = 0; index < kMaxPivots; ++index)
                if (!found && index < col_count && differing == col_dict[index]) {
                    found = true; differing_index = index;
                }
            if (!found && col_count < matrix_size) {
                differing_index = col_count; col_dict[col_count++] = differing; found = true;
            }
            if (found) matrix[pointer][differing_index] = true;
            else for (unsigned index = 0; index < col_count; ++index) matrix[pointer][index] = true;
        }
    }
    for (unsigned pattern_id = 1; pattern_id <= candidate_count(rows, cols); ++pattern_id) {
        const uint8_t pattern = masks.at(fixed_mask_key(rows, cols, pattern_id));
        bool valid = !state.overflow && !dictionary_overflow;
        for (unsigned row = 0; row < matrix_size; ++row)
            for (unsigned col = 0; col < matrix_size; ++col)
                if (matrix[row][col] && ((pattern >> row) & 1U) && !((pattern >> col) & 1U))
                    valid = false;
        if (valid) return {true, static_cast<uint8_t>(pattern_id)};
    }
    return {false, 0U};
}

AnalyzerState generate_random_state(uint32_t& seed, unsigned matrix_size) {
    AnalyzerState state;
    const unsigned compact_pivots = next_random(seed) % (matrix_size + 1U);
    for (unsigned index = 0; index < kMaxPivots; ++index) {
        state.pivot_valid[index] = index < compact_pivots;
        state.pivot_rows[index] = next_random(seed) & 0x1ffU;
        state.pivot_cols[index] = next_random(seed) & 0x1fffU;
        for (unsigned level = 0; level < kThresholdLevels; ++level) {
            state.row_gt[level][index] = (next_random(seed) & 3U) == 0U;
            state.col_gt[level][index] = (next_random(seed) & 3U) == 0U;
        }
    }
    for (unsigned index = 0; index < kHybridEntries; ++index) {
        state.hybrid_valid[index] = (next_random(seed) & 1U) != 0U;
        state.hybrid_pointer[index] = next_random(seed) & 7U;
        state.hybrid_descriptor[index] = (next_random(seed) & 1U) != 0U;
        state.hybrid_differing[index] = next_random(seed) & 0x1fffU;
    }
    state.overflow = (next_random(seed) & 7U) == 0U;
    return state;
}

}  // namespace n3_rc_analyzer_oracle
