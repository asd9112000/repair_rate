#ifndef N3_RC_ANALYZER_ORACLE_HPP
#define N3_RC_ANALYZER_ORACLE_HPP

#include <array>
#include <cstdint>
#include <map>
#include <string>

namespace n3_rc_analyzer_oracle {

constexpr unsigned kMaxPivots = 7U;
constexpr unsigned kThresholdLevels = 4U;
constexpr unsigned kHybridEntries = 17U;
constexpr unsigned kRowAddressWidth = 9U;
constexpr unsigned kPhysicalColumnAddressWidth = 13U;
constexpr unsigned kHybridLineAddressWidth = 13U;
constexpr unsigned kPatternIdWidth = 6U;
constexpr unsigned kSemanticStateWidth =
    kMaxPivots + kMaxPivots * kRowAddressWidth +
    kMaxPivots * kPhysicalColumnAddressWidth +
    2U * kThresholdLevels * kMaxPivots + kHybridEntries +
    kHybridEntries * 3U + kHybridEntries +
    kHybridEntries * kHybridLineAddressWidth + 1U;
static_assert(kSemanticStateWidth == 524U, "N3 analyzer state width must remain 524 bits");

struct AnalyzerState {
    std::array<bool, kMaxPivots> pivot_valid{};
    std::array<uint16_t, kMaxPivots> pivot_rows{};
    std::array<uint16_t, kMaxPivots> pivot_cols{};
    std::array<std::array<bool, kMaxPivots>, kThresholdLevels> row_gt{};
    std::array<std::array<bool, kMaxPivots>, kThresholdLevels> col_gt{};
    std::array<bool, kHybridEntries> hybrid_valid{};
    std::array<uint8_t, kHybridEntries> hybrid_pointer{};
    std::array<bool, kHybridEntries> hybrid_descriptor{};
    std::array<uint16_t, kHybridEntries> hybrid_differing{};
    bool overflow = false;
};

struct CapacityRequest {
    unsigned rows;
    unsigned cols;
    bool transpose;
};

struct AnalyzerResult {
    bool repairable;
    uint8_t pattern;
};

using FixedMaskMap = std::map<std::string, uint8_t>;

uint32_t next_random(uint32_t& state);
std::string fixed_mask_key(unsigned rows, unsigned cols, unsigned pattern);
FixedMaskMap load_fixed_masks(const std::string& csv_path);
unsigned candidate_count(unsigned rows, unsigned cols);
AnalyzerResult evaluate(const AnalyzerState& state, const CapacityRequest& request,
                        const FixedMaskMap& masks);
AnalyzerState generate_random_state(uint32_t& seed, unsigned matrix_size);

}  // namespace n3_rc_analyzer_oracle

#endif
