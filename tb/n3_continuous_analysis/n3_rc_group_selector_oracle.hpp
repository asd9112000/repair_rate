#ifndef N3_RC_GROUP_SELECTOR_ORACLE_HPP
#define N3_RC_GROUP_SELECTOR_ORACLE_HPP

#include <array>
#include <cstdint>

namespace n3_rc_group_selector_oracle {

constexpr unsigned kSubarrays = 4U;
constexpr unsigned kRecordsPerSubarray = 4U;
constexpr unsigned kRecordCount = kSubarrays * kRecordsPerSubarray;
constexpr unsigned kPatternIdWidth = 6U;
constexpr unsigned kRecordWidth = 1U + kPatternIdWidth;
constexpr unsigned kGroupStoreWidth = kRecordCount * kRecordWidth;
static_assert(kGroupStoreWidth == 112U, "N3 GROUP store width must remain 112 bits");

enum class Action : uint8_t {
    Local = 0U,
    ReleaseOnly = 1U,
    BorrowOnly = 2U,
    ReleaseAndBorrow = 3U,
};

struct Candidate {
    bool valid = false;
    uint8_t pattern_id = 0U;
};

using CandidateStore =
    std::array<std::array<Candidate, kRecordsPerSubarray>, kSubarrays>;

struct Selection {
    bool repairable = false;
    std::array<uint8_t, kSubarrays> slot{};
    std::array<uint8_t, kSubarrays> config_id{};
    std::array<uint8_t, kSubarrays> pattern_id{};
    std::array<Action, kSubarrays> action{};
    std::array<uint8_t, kSubarrays> donor{};
    std::array<bool, kSubarrays> borrow{};
    std::array<bool, kSubarrays> release{};
};

Action action_for_slot(unsigned slot);
uint8_t config_for_slot(unsigned subarray, unsigned slot);
bool resource_legal(const std::array<uint8_t, kSubarrays>& slots);
Selection select(const CandidateStore& candidates);

}  // namespace n3_rc_group_selector_oracle

#endif
