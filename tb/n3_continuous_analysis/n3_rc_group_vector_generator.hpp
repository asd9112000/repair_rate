#ifndef N3_RC_GROUP_VECTOR_GENERATOR_HPP
#define N3_RC_GROUP_VECTOR_GENERATOR_HPP

#include "n3_rc_analyzer_oracle.hpp"
#include "n3_rc_group_selector_oracle.hpp"
#include "n3_rc_reconstruction_oracle.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace n3_rc_group_vector_generator {

enum class GenerationMode {
    General,
    Repairable,
    Unrepairable,
    HighPattern,
    SeventhPivot,
    HybridAboveSeven,
    PhysicalColumnWide,
};

struct GeneratedCase {
    uint32_t seed = 0U;
    uint64_t case_index = 0U;
    GenerationMode mode = GenerationMode::General;
    std::array<n3_rc_analyzer_oracle::AnalyzerState, 4> analyzer_state{};
    n3_rc_group_selector_oracle::CandidateStore candidates{};
    n3_rc_group_selector_oracle::Selection selection{};
    n3_rc_reconstruction_oracle::RetainedState retained{};
    n3_rc_reconstruction_oracle::RepairResult repairs{};
    bool exercises_seventh_pivot = false;
    bool exercises_hybrid_above_seven = false;
    bool exercises_high_pattern = false;
    bool exercises_wide_physical_column = false;
};

const char* mode_name(GenerationMode mode);
GenerationMode parse_mode(const std::string& text);
GenerationMode smoke_mode(uint64_t case_index);
GeneratedCase generate(uint32_t seed, uint64_t case_index, GenerationMode mode,
                       const n3_rc_analyzer_oracle::FixedMaskMap& masks);
bool equal(const GeneratedCase& left, const GeneratedCase& right);

}  // namespace n3_rc_group_vector_generator

#endif
