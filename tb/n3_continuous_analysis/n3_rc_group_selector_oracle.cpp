#include "n3_rc_group_selector_oracle.hpp"

namespace n3_rc_group_selector_oracle {

Action action_for_slot(unsigned slot) {
    static constexpr std::array<Action, kRecordsPerSubarray> kActions = {
        Action::Local, Action::ReleaseOnly, Action::BorrowOnly,
        Action::ReleaseAndBorrow};
    return kActions.at(slot);
}

uint8_t config_for_slot(unsigned subarray, unsigned slot) {
    if (slot == 0U) return 0U;
    return static_cast<uint8_t>((subarray == 0U || subarray == 3U) ? slot : slot + 3U);
}

bool resource_legal(const std::array<uint8_t, kSubarrays>& slots) {
    // Directional ring: A borrows B's release, B borrows D's release,
    // C borrows A's release, and D borrows C's release.
    static constexpr std::array<unsigned, kSubarrays> kDonor = {1U, 3U, 0U, 2U};
    for (unsigned recipient = 0; recipient < kSubarrays; ++recipient) {
        const bool borrows = (slots[recipient] & 2U) != 0U;
        const bool donor_releases = (slots[kDonor[recipient]] & 1U) != 0U;
        if (borrows && !donor_releases) return false;
    }
    return true;
}

Selection select(const CandidateStore& candidates) {
    Selection result;
    static constexpr std::array<uint8_t, kSubarrays> kReportedDonor = {2U, 1U, 0U, 3U};
    for (unsigned d = 0; d < kRecordsPerSubarray; ++d)
        for (unsigned c = 0; c < kRecordsPerSubarray; ++c)
            for (unsigned b = 0; b < kRecordsPerSubarray; ++b)
                for (unsigned a = 0; a < kRecordsPerSubarray; ++a) {
                    const std::array<uint8_t, kSubarrays> slots = {
                        static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                        static_cast<uint8_t>(c), static_cast<uint8_t>(d)};
                    if (!resource_legal(slots)) continue;
                    bool all_valid = true;
                    for (unsigned sa = 0; sa < kSubarrays; ++sa)
                        all_valid = all_valid && candidates[sa][slots[sa]].valid;
                    if (!all_valid) continue;
                    result.repairable = true;
                    result.slot = slots;
                    result.donor = kReportedDonor;
                    for (unsigned sa = 0; sa < kSubarrays; ++sa) {
                        result.config_id[sa] = config_for_slot(sa, slots[sa]);
                        result.pattern_id[sa] = candidates[sa][slots[sa]].pattern_id;
                        result.action[sa] = action_for_slot(slots[sa]);
                        result.release[sa] = (slots[sa] & 1U) != 0U;
                        result.borrow[sa] = (slots[sa] & 2U) != 0U;
                    }
                    return result;
                }
    return result;
}

}  // namespace n3_rc_group_selector_oracle
