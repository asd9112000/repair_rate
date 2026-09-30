#include "n3_rc_reconstruction_oracle.hpp"

#include <stdexcept>

namespace n3_rc_reconstruction_oracle {
namespace {

struct ConfigShape {
    unsigned rows;
    unsigned cols;
    unsigned active_slots;
    bool transpose;
};

ConfigShape config_shape(unsigned config) {
    switch (config) {
        case 0U: return {3U, 3U, 6U, false};
        case 1U: return {3U, 2U, 5U, true};
        case 2U: return {4U, 3U, 7U, true};
        case 3U: return {4U, 2U, 6U, true};
        case 4U: return {3U, 2U, 5U, false};
        case 5U: return {4U, 3U, 7U, false};
        case 6U: return {4U, 2U, 6U, false};
        default: throw std::runtime_error("invalid N3 reconstruction ConfigID");
    }
}

}  // namespace

RepairResult reconstruct(const RetainedState& state,
                         const n3_rc_analyzer_oracle::FixedMaskMap& masks) {
    RepairResult result{};
    for (unsigned sa = 0; sa < kSubarrays; ++sa) {
        if (!state.commit_valid[sa]) continue;
        const ConfigShape shape = config_shape(state.selected_config[sa]);
        const auto key = n3_rc_analyzer_oracle::fixed_mask_key(
            shape.rows, shape.cols, state.selected_pattern[sa]);
        const auto found = masks.find(key);
        if (found == masks.end()) throw std::runtime_error("unknown N3 reconstruction PatternID");
        const uint8_t mask = found->second;
        for (unsigned slot = 0; slot < shape.active_slots; ++slot) {
            const bool mask_is_row = ((mask >> slot) & 1U) != 0U;
            const bool is_row = shape.transpose ? !mask_is_row : mask_is_row;
            result[sa][slot].valid = true;
            result[sa][slot].is_row = is_row;
            result[sa][slot].address = is_row ? state.pivot_rows[sa][slot]
                                              : state.pivot_cols[sa][slot];
        }
    }
    return result;
}

bool equal(const RepairResult& left, const RepairResult& right) {
    for (unsigned sa = 0; sa < kSubarrays; ++sa)
        for (unsigned slot = 0; slot < kPivotSlots; ++slot)
            if (left[sa][slot].valid != right[sa][slot].valid ||
                left[sa][slot].is_row != right[sa][slot].is_row ||
                left[sa][slot].address != right[sa][slot].address)
                return false;
    return true;
}

}  // namespace n3_rc_reconstruction_oracle
