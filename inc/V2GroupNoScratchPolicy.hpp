#ifndef DYNAMIC_SPARE_SHARING_V2_GROUP_NO_SCRATCH_POLICY_HPP
#define DYNAMIC_SPARE_SHARING_V2_GROUP_NO_SCRATCH_POLICY_HPP

#include <array>
#include <cstddef>
#include <stdexcept>

#include "SimulationConfig.hpp"

namespace dynamic_spare
{
enum class V2GroupAction { Local, ReleaseOnly, BorrowOnly, ReleaseAndBorrow };
struct V2RoleSlotMapping
{
    char role;
    std::size_t roleSlot;
    int configId;
    V2GroupAction action;
};

// Array position is the historical C++ priority. ConfigID is encoding only.
constexpr std::array<V2RoleSlotMapping, 4> v2RoleSlotMappings(std::size_t sa)
{
    return sa == 0 || sa == 3
        ? std::array<V2RoleSlotMapping, 4>{{
            {sa == 0 ? 'A' : 'D', 0, 0, V2GroupAction::Local},
            {sa == 0 ? 'A' : 'D', 1, 1, V2GroupAction::ReleaseOnly},
            {sa == 0 ? 'A' : 'D', 2, 2, V2GroupAction::BorrowOnly},
            {sa == 0 ? 'A' : 'D', 3, 3, V2GroupAction::ReleaseAndBorrow}}}
        : std::array<V2RoleSlotMapping, 4>{{
            {sa == 1 ? 'B' : 'C', 0, 0, V2GroupAction::Local},
            {sa == 1 ? 'B' : 'C', 1, 4, V2GroupAction::ReleaseOnly},
            {sa == 1 ? 'B' : 'C', 2, 5, V2GroupAction::BorrowOnly},
            {sa == 1 ? 'B' : 'C', 3, 6, V2GroupAction::ReleaseAndBorrow}}};
}

// The frozen DATE 2x2,m=1 RTL uses the legacy adapter's numeric IDs.  A/D
// encode the row-release family as 4/5/6; B/C encode the column-release family
// as 1/2/3. This differs intentionally from the isolated RS3 target table.
constexpr std::array<V2RoleSlotMapping, 4>
v2FrozenDate2x2RoleSlotMappings(std::size_t sa)
{
    return sa == 0 || sa == 3
        ? std::array<V2RoleSlotMapping, 4>{{
            {sa == 0 ? 'A' : 'D', 0, 0, V2GroupAction::Local},
            {sa == 0 ? 'A' : 'D', 1, 4, V2GroupAction::ReleaseOnly},
            {sa == 0 ? 'A' : 'D', 2, 5, V2GroupAction::BorrowOnly},
            {sa == 0 ? 'A' : 'D', 3, 6, V2GroupAction::ReleaseAndBorrow}}}
        : std::array<V2RoleSlotMapping, 4>{{
            {sa == 1 ? 'B' : 'C', 0, 0, V2GroupAction::Local},
            {sa == 1 ? 'B' : 'C', 1, 1, V2GroupAction::ReleaseOnly},
            {sa == 1 ? 'B' : 'C', 2, 2, V2GroupAction::BorrowOnly},
            {sa == 1 ? 'B' : 'C', 3, 3, V2GroupAction::ReleaseAndBorrow}}};
}

inline ConfigContractVersion canonicalV2ConfigContract(
    const SimulationConfig &config)
{
    if (config.layout != GroupLayout::Grid2x2 ||
        (config.topology != SharingTopology::NoSharing &&
         (config.topology != SharingTopology::Directional ||
          config.sharedRows != 1 || config.sharedColumns != 1)))
    {
        throw std::invalid_argument(
            "GROUP_GREEDY_RTL_CANONICAL requires 2x2 directional m=1");
    }
    if (config.spareRows == 2 && config.spareColumns == 2)
        return ConfigContractVersion::FrozenDate2x2M1;
    if (config.spareRows == 3 && config.spareColumns == 3)
        return ConfigContractVersion::Rs3Cs3M1;
    throw std::invalid_argument(
        "Directional V2 policy has no frozen ConfigID contract for this Rs/Cs");
}

constexpr std::array<V2RoleSlotMapping, 4> v2RoleSlotMappings(
    ConfigContractVersion contract,
    std::size_t sa)
{
    return contract == ConfigContractVersion::FrozenDate2x2M1
        ? v2FrozenDate2x2RoleSlotMappings(sa)
        : v2RoleSlotMappings(sa);
}

// The active GROUP RTL reads candidate slots in rank order 1, 0, 3, 2:
// RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY. The role-slot and
// ConfigID encoding remain the frozen per-role table above.
constexpr std::array<V2RoleSlotMapping, 4>
v2RtlGroupRoleSlotMappings(ConfigContractVersion contract, std::size_t sa)
{
    const auto slots = v2RoleSlotMappings(contract, sa);
    return {{slots[1], slots[0], slots[3], slots[2]}};
}
const char *toString(V2GroupAction action) noexcept;
} // namespace dynamic_spare
#endif
