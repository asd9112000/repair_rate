#ifndef N3_RC_RECONSTRUCTION_ORACLE_HPP
#define N3_RC_RECONSTRUCTION_ORACLE_HPP

#include "n3_rc_analyzer_oracle.hpp"

#include <array>
#include <cstdint>

namespace n3_rc_reconstruction_oracle {

constexpr unsigned kSubarrays = 4U;
constexpr unsigned kPivotSlots = 7U;
constexpr unsigned kRowAddressWidth = 9U;
constexpr unsigned kPhysicalColumnAddressWidth = 13U;
constexpr unsigned kPrivatePivotCaptureWidth =
    kSubarrays * kPivotSlots * (kRowAddressWidth + kPhysicalColumnAddressWidth);
constexpr unsigned kCommitValidWidth = 4U;
constexpr unsigned kSelectedConfigWidth = 12U;
constexpr unsigned kSelectedPatternWidth = 24U;
constexpr unsigned kOtherMetadataWidth =
    kCommitValidWidth + kSelectedConfigWidth + kSelectedPatternWidth;
constexpr unsigned kRetainedWidth = kPrivatePivotCaptureWidth + kOtherMetadataWidth;
static_assert(kPrivatePivotCaptureWidth == 616U,
              "N3 private pivot capture width must remain 616 bits");
static_assert(kOtherMetadataWidth == 40U,
              "N3 reconstruction metadata width must remain 40 bits");
static_assert(kRetainedWidth == 656U,
              "N3 reconstruction retained width must remain 656 bits");

struct RetainedState {
    std::array<std::array<uint16_t, kPivotSlots>, kSubarrays> pivot_rows{};
    std::array<std::array<uint16_t, kPivotSlots>, kSubarrays> pivot_cols{};
    std::array<bool, kSubarrays> commit_valid{};
    std::array<uint8_t, kSubarrays> selected_config{};
    std::array<uint8_t, kSubarrays> selected_pattern{};
};

struct RepairLine {
    bool valid = false;
    bool is_row = false;
    uint16_t address = 0U;
};

using RepairResult = std::array<std::array<RepairLine, kPivotSlots>, kSubarrays>;

RepairResult reconstruct(const RetainedState& state,
                         const n3_rc_analyzer_oracle::FixedMaskMap& masks);
bool equal(const RepairResult& left, const RepairResult& right);

}  // namespace n3_rc_reconstruction_oracle

#endif
