#include "n3_rc_group_vector_generator.hpp"

#include <stdexcept>

namespace n3_rc_group_vector_generator {
namespace {

using n3_rc_analyzer_oracle::AnalyzerResult;
using n3_rc_analyzer_oracle::AnalyzerState;
using n3_rc_analyzer_oracle::CapacityRequest;
using n3_rc_analyzer_oracle::FixedMaskMap;

uint32_t mixed_seed(uint32_t seed, uint64_t case_index, unsigned stream) {
    uint32_t value = seed ^ static_cast<uint32_t>(case_index) ^
                     static_cast<uint32_t>(case_index >> 32U) ^
                     (0x9e3779b9U * (stream + 1U));
    n3_rc_analyzer_oracle::next_random(value);
    return value;
}

AnalyzerState simple_repairable_state(uint32_t seed) {
    AnalyzerState state{};
    for (unsigned slot = 0; slot < 7; ++slot) {
        state.pivot_valid[slot] = true;
        state.pivot_rows[slot] = static_cast<uint16_t>((seed + 17U * slot) & 0x1ffU);
        state.pivot_cols[slot] = static_cast<uint16_t>((seed + 257U * slot) & 0x1fffU);
    }
    return state;
}

CapacityRequest request_for(unsigned subarray, unsigned slot) {
    if (slot == 0U) return {3U, 3U, false};
    const bool transpose = subarray == 0U || subarray == 3U;
    if (slot == 1U) return transpose ? CapacityRequest{2U, 3U, true}
                                     : CapacityRequest{3U, 2U, false};
    if (slot == 2U) return transpose ? CapacityRequest{3U, 4U, true}
                                     : CapacityRequest{4U, 3U, false};
    return transpose ? CapacityRequest{2U, 4U, true}
                     : CapacityRequest{4U, 2U, false};
}

AnalyzerState high_pattern_state(uint32_t seed, const FixedMaskMap& masks) {
    uint32_t rng = seed;
    for (unsigned attempt = 0; attempt < 1000000U; ++attempt) {
        AnalyzerState state = n3_rc_analyzer_oracle::generate_random_state(rng, 6U);
        const AnalyzerResult result = n3_rc_analyzer_oracle::evaluate(
            state, {3U, 3U, false}, masks);
        if (result.repairable && result.pattern > 15U) return state;
    }
    throw std::runtime_error("no deterministic PatternID > 15 witness");
}

AnalyzerState seventh_pivot_borrower(uint32_t seed, const FixedMaskMap& masks) {
    uint32_t rng = seed;
    for (unsigned attempt = 0; attempt < 1000000U; ++attempt) {
        AnalyzerState state = n3_rc_analyzer_oracle::generate_random_state(rng, 7U);
        const AnalyzerResult local = n3_rc_analyzer_oracle::evaluate(
            state, {3U, 3U, false}, masks);
        const AnalyzerResult release = n3_rc_analyzer_oracle::evaluate(
            state, {2U, 3U, true}, masks);
        const AnalyzerResult borrow = n3_rc_analyzer_oracle::evaluate(
            state, {3U, 4U, true}, masks);
        if (!local.repairable && !release.repairable && borrow.repairable &&
            state.pivot_valid[6])
            return state;
    }
    throw std::runtime_error("no deterministic seventh-pivot borrower witness");
}

void build_expectations(GeneratedCase& generated, const FixedMaskMap& masks) {
    for (unsigned sa = 0; sa < 4; ++sa) {
        for (unsigned slot = 0; slot < 4; ++slot) {
            const AnalyzerResult result = n3_rc_analyzer_oracle::evaluate(
                generated.analyzer_state[sa], request_for(sa, slot), masks);
            generated.candidates[sa][slot] = {result.repairable, result.pattern};
        }
    }
    generated.selection = n3_rc_group_selector_oracle::select(generated.candidates);
    for (unsigned sa = 0; sa < 4; ++sa) {
        generated.retained.commit_valid[sa] = generated.selection.repairable;
        generated.retained.selected_config[sa] = generated.selection.config_id[sa];
        generated.retained.selected_pattern[sa] = generated.selection.pattern_id[sa];
        for (unsigned slot = 0; slot < 7; ++slot) {
            generated.retained.pivot_rows[sa][slot] =
                generated.analyzer_state[sa].pivot_rows[slot];
            generated.retained.pivot_cols[sa][slot] =
                generated.analyzer_state[sa].pivot_cols[slot];
        }
    }
    generated.repairs = n3_rc_reconstruction_oracle::reconstruct(generated.retained, masks);
    for (unsigned sa = 0; sa < 4; ++sa) {
        generated.exercises_seventh_pivot = generated.exercises_seventh_pivot ||
            (generated.analyzer_state[sa].pivot_valid[6] &&
             (generated.selection.config_id[sa] == 2U ||
              generated.selection.config_id[sa] == 5U));
        generated.exercises_high_pattern = generated.exercises_high_pattern ||
            generated.selection.pattern_id[sa] > 15U;
        for (unsigned index = 8; index < 17; ++index)
            generated.exercises_hybrid_above_seven =
                generated.exercises_hybrid_above_seven ||
                generated.analyzer_state[sa].hybrid_valid[index];
        for (unsigned slot = 0; slot < 7; ++slot)
            generated.exercises_wide_physical_column =
                generated.exercises_wide_physical_column ||
                (generated.repairs[sa][slot].valid &&
                 !generated.repairs[sa][slot].is_row &&
                 generated.repairs[sa][slot].address > 255U);
    }
}

bool equal_state(const AnalyzerState& left, const AnalyzerState& right) {
    return left.pivot_valid == right.pivot_valid &&
           left.pivot_rows == right.pivot_rows && left.pivot_cols == right.pivot_cols &&
           left.row_gt == right.row_gt && left.col_gt == right.col_gt &&
           left.hybrid_valid == right.hybrid_valid &&
           left.hybrid_pointer == right.hybrid_pointer &&
           left.hybrid_descriptor == right.hybrid_descriptor &&
           left.hybrid_differing == right.hybrid_differing &&
           left.overflow == right.overflow;
}

}  // namespace

const char* mode_name(GenerationMode mode) {
    switch (mode) {
        case GenerationMode::General: return "GENERAL";
        case GenerationMode::Repairable: return "REPAIRABLE";
        case GenerationMode::Unrepairable: return "UNREPAIRABLE";
        case GenerationMode::HighPattern: return "HIGH_PATTERN";
        case GenerationMode::SeventhPivot: return "SEVENTH_PIVOT";
        case GenerationMode::HybridAboveSeven: return "HYBRID_GT_7";
        case GenerationMode::PhysicalColumnWide: return "PHYSICAL_COLUMN_WIDE";
    }
    throw std::runtime_error("invalid generation mode");
}

GenerationMode parse_mode(const std::string& text) {
    if (text == "GENERAL") return GenerationMode::General;
    if (text == "REPAIRABLE") return GenerationMode::Repairable;
    if (text == "UNREPAIRABLE") return GenerationMode::Unrepairable;
    if (text == "HIGH_PATTERN") return GenerationMode::HighPattern;
    if (text == "SEVENTH_PIVOT") return GenerationMode::SeventhPivot;
    if (text == "HYBRID_GT_7") return GenerationMode::HybridAboveSeven;
    if (text == "PHYSICAL_COLUMN_WIDE") return GenerationMode::PhysicalColumnWide;
    throw std::runtime_error("unknown generation mode: " + text);
}

GenerationMode smoke_mode(uint64_t case_index) {
    switch (case_index) {
        case 0U: return GenerationMode::Repairable;
        case 1U: return GenerationMode::Unrepairable;
        case 2U: return GenerationMode::HighPattern;
        case 3U: return GenerationMode::SeventhPivot;
        case 4U: return GenerationMode::HybridAboveSeven;
        case 5U: return GenerationMode::PhysicalColumnWide;
        default: return GenerationMode::General;
    }
}

GeneratedCase generate(uint32_t seed, uint64_t case_index, GenerationMode mode,
                       const FixedMaskMap& masks) {
    GeneratedCase generated;
    generated.seed = seed;
    generated.case_index = case_index;
    generated.mode = mode;
    if (mode == GenerationMode::General) {
        for (unsigned sa = 0; sa < 4; ++sa) {
            uint32_t rng = mixed_seed(seed, case_index, sa);
            generated.analyzer_state[sa] =
                n3_rc_analyzer_oracle::generate_random_state(rng, 7U);
        }
    } else if (mode == GenerationMode::Unrepairable) {
        for (unsigned sa = 0; sa < 4; ++sa) {
            generated.analyzer_state[sa] =
                simple_repairable_state(mixed_seed(seed, case_index, sa));
            generated.analyzer_state[sa].overflow = true;
        }
    } else if (mode == GenerationMode::HighPattern) {
        const AnalyzerState state = high_pattern_state(seed, masks);
        generated.analyzer_state.fill(state);
    } else if (mode == GenerationMode::SeventhPivot) {
        generated.analyzer_state[0] = seventh_pivot_borrower(
            mixed_seed(seed, case_index, 0U), masks);
        for (unsigned sa = 1; sa < 4; ++sa)
            generated.analyzer_state[sa] =
                simple_repairable_state(mixed_seed(seed, case_index, sa));
    } else {
        for (unsigned sa = 0; sa < 4; ++sa)
            generated.analyzer_state[sa] =
                simple_repairable_state(mixed_seed(seed, case_index, sa));
        if (mode == GenerationMode::HybridAboveSeven) {
            AnalyzerState& state = generated.analyzer_state[0];
            state.hybrid_valid[8] = true;
            state.hybrid_pointer[8] = 0U;
            state.hybrid_descriptor[8] = false;
            state.hybrid_differing[8] = state.pivot_cols[0];
        } else if (mode == GenerationMode::PhysicalColumnWide) {
            for (AnalyzerState& state : generated.analyzer_state) {
                state.pivot_cols[0] = 1U;
                state.pivot_cols[1] = 257U;
                state.pivot_cols[6] = 8191U;
            }
        }
    }
    build_expectations(generated, masks);
    if (mode == GenerationMode::Repairable && !generated.selection.repairable)
        throw std::runtime_error("directed repairable case is not repairable");
    if (mode == GenerationMode::Unrepairable && generated.selection.repairable)
        throw std::runtime_error("directed unrepairable case is repairable");
    if (mode == GenerationMode::HighPattern && !generated.exercises_high_pattern)
        throw std::runtime_error("high-pattern case did not select PatternID > 15");
    if (mode == GenerationMode::SeventhPivot && !generated.exercises_seventh_pivot)
        throw std::runtime_error("seventh-pivot case did not select a seven-slot Config");
    if (mode == GenerationMode::HybridAboveSeven &&
        !generated.exercises_hybrid_above_seven)
        throw std::runtime_error("hybrid-above-seven case did not exercise entry 8");
    if (mode == GenerationMode::PhysicalColumnWide &&
        !generated.exercises_wide_physical_column)
        throw std::runtime_error("wide-column case did not reconstruct an address above 255");
    return generated;
}

bool equal(const GeneratedCase& left, const GeneratedCase& right) {
    if (left.seed != right.seed || left.case_index != right.case_index ||
        left.mode != right.mode || left.selection.repairable != right.selection.repairable ||
        left.selection.slot != right.selection.slot ||
        left.selection.config_id != right.selection.config_id ||
        left.selection.pattern_id != right.selection.pattern_id ||
        left.selection.action != right.selection.action ||
        left.selection.donor != right.selection.donor ||
        left.selection.borrow != right.selection.borrow ||
        left.selection.release != right.selection.release ||
        left.exercises_seventh_pivot != right.exercises_seventh_pivot ||
        left.exercises_hybrid_above_seven != right.exercises_hybrid_above_seven ||
        left.exercises_high_pattern != right.exercises_high_pattern ||
        left.exercises_wide_physical_column != right.exercises_wide_physical_column)
        return false;
    for (unsigned sa = 0; sa < 4; ++sa) {
        if (!equal_state(left.analyzer_state[sa], right.analyzer_state[sa])) return false;
        for (unsigned slot = 0; slot < 4; ++slot)
            if (left.candidates[sa][slot].valid != right.candidates[sa][slot].valid ||
                left.candidates[sa][slot].pattern_id !=
                    right.candidates[sa][slot].pattern_id)
                return false;
    }
    return n3_rc_reconstruction_oracle::equal(left.repairs, right.repairs) &&
           left.retained.pivot_rows == right.retained.pivot_rows &&
           left.retained.pivot_cols == right.retained.pivot_cols &&
           left.retained.commit_valid == right.retained.commit_valid &&
           left.retained.selected_config == right.retained.selected_config &&
           left.retained.selected_pattern == right.retained.selected_pattern;
}

}  // namespace n3_rc_group_vector_generator
