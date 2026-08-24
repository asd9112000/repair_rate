#include "../inc/DynamicFaultGenerator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace dynamic_spare
{

class DynamicFaultGenerator::StableRandom
{
public:
    explicit StableRandom(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next()
    {
        state_ += 0x9e3779b97f4a7c15ULL;
        std::uint64_t value = state_;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    std::uint64_t bounded(std::uint64_t upperExclusive)
    {
        if (upperExclusive == 0)
        {
            throw std::invalid_argument("Random bound must be positive");
        }
        const std::uint64_t threshold =
            static_cast<std::uint64_t>(-upperExclusive) % upperExclusive;
        for (;;)
        {
            const std::uint64_t value = next();
            if (value >= threshold)
            {
                return value % upperExclusive;
            }
        }
    }

private:
    std::uint64_t state_;
};

namespace
{

std::array<std::uint64_t, kSubarrayCount> modelWeights(
    FaultCountModel model)
{
    switch (model)
    {
        case FaultCountModel::FileProvided:
            break;
        case FaultCountModel::Uniform:
            return {{1, 1, 1, 1}};
        case FaultCountModel::ModerateImbalance:
            return {{2, 4, 5, 9}};
        case FaultCountModel::StrongImbalance:
            return {{1, 2, 3, 14}};
        case FaultCountModel::Hotspot:
            return {{1, 1, 1, 17}};
        case FaultCountModel::UserDefined:
            break;
    }
    return {{0, 0, 0, 0}};
}

std::uint64_t addressKey(
    std::uint32_t row,
    std::uint32_t column,
    std::uint32_t columns)
{
    return static_cast<std::uint64_t>(row) * columns + column;
}

} // namespace

DynamicFaultGenerator::DynamicFaultGenerator(
    const SimulationConfig &config)
    : config_(config)
{
    config_.validate();
    if (config_.faultCountModel == FaultCountModel::FileProvided)
    {
        throw std::invalid_argument(
            "File-provided fault metadata cannot generate new faults");
    }
    if (config_.faultSpatialModel == FaultSpatialModel::FileProvided)
    {
        throw std::invalid_argument(
            "File-provided spatial metadata cannot generate new faults");
    }
    if (config_.memoryRows >
            static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        config_.memoryColumns >
            static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
    {
        throw std::invalid_argument(
            "Fault addresses exceed the legacy signed-int address range");
    }
}

std::vector<std::size_t> DynamicFaultGenerator::countsForGroup() const
{
    std::vector<std::size_t> counts(kSubarrayCount, 0);
    if (config_.faultCountModel == FaultCountModel::UserDefined)
    {
        for (std::size_t subarray = 0;
             subarray < kSubarrayCount; ++subarray)
        {
            counts[subarray] = static_cast<std::size_t>(
                config_.userDefinedFaultCounts[subarray]);
        }
        return counts;
    }

    const auto weights = modelWeights(config_.faultCountModel);
    std::uint64_t weightTotal = 0;
    for (std::uint64_t weight : weights)
    {
        weightTotal += weight;
    }

    struct Fraction
    {
        std::uint64_t remainder = 0;
        std::size_t subarray = 0;
    };
    std::array<Fraction, kSubarrayCount> fractions;
    std::uint64_t assigned = 0;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        const std::uint64_t whole =
            (config_.faultCount / weightTotal) * weights[subarray];
        const std::uint64_t fractionalNumerator =
            (config_.faultCount % weightTotal) * weights[subarray];
        counts[subarray] = static_cast<std::size_t>(
            whole + fractionalNumerator / weightTotal);
        assigned += counts[subarray];
        fractions[subarray] = Fraction{
            fractionalNumerator % weightTotal, subarray};
    }
    std::sort(
        fractions.begin(), fractions.end(),
        [](const Fraction &left, const Fraction &right)
        {
            if (left.remainder != right.remainder)
            {
                return left.remainder > right.remainder;
            }
            return left.subarray < right.subarray;
        });
    for (std::size_t index = 0;
         assigned < config_.faultCount; ++index, ++assigned)
    {
        ++counts[fractions[index % kSubarrayCount].subarray];
    }
    return counts;
}

std::vector<Fault> DynamicFaultGenerator::faultsForSubarray(
    std::size_t count,
    std::size_t runIndex,
    std::size_t subarray,
    StableRandom &random) const
{
    const std::uint64_t cellCount =
        static_cast<std::uint64_t>(config_.memoryRows) *
        config_.memoryColumns;
    if (count > cellCount)
    {
        throw std::invalid_argument(
            "Fault count exceeds the number of unique memory cells");
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> addresses;
    addresses.reserve(count);
    std::set<std::uint64_t> used;
    const auto randomAddress = [&]()
    {
        return std::make_pair(
            static_cast<std::uint32_t>(random.bounded(config_.memoryRows)),
            static_cast<std::uint32_t>(random.bounded(config_.memoryColumns)));
    };

    while (addresses.size() < count)
    {
        std::pair<std::uint32_t, std::uint32_t> address;
        const std::uint64_t action = random.bounded(100);
        const bool clustered =
            config_.faultSpatialModel == FaultSpatialModel::Clustered;
        const bool mixed =
            config_.faultSpatialModel == FaultSpatialModel::Mixed;
        const std::uint64_t clusterThreshold = clustered ? 70 : (mixed ? 20 : 0);
        const std::uint64_t sameLineThreshold = clustered ? 90 : (mixed ? 50 : 0);

        if (!addresses.empty() && action < clusterThreshold)
        {
            const auto &base = addresses[static_cast<std::size_t>(
                random.bounded(addresses.size()))];
            const int rowOffset = static_cast<int>(random.bounded(5)) - 2;
            const int columnOffset =
                static_cast<int>(random.bounded(5)) - 2;
            const int row = std::max(
                0,
                std::min(
                    static_cast<int>(config_.memoryRows) - 1,
                    static_cast<int>(base.first) + rowOffset));
            const int column = std::max(
                0,
                std::min(
                    static_cast<int>(config_.memoryColumns) - 1,
                    static_cast<int>(base.second) + columnOffset));
            address = {
                static_cast<std::uint32_t>(row),
                static_cast<std::uint32_t>(column)};
        }
        else if (!addresses.empty() && action < sameLineThreshold)
        {
            const auto &base = addresses[static_cast<std::size_t>(
                random.bounded(addresses.size()))];
            if (random.bounded(2) == 0)
            {
                address = {
                    base.first,
                    static_cast<std::uint32_t>(
                        random.bounded(config_.memoryColumns))};
            }
            else
            {
                address = {
                    static_cast<std::uint32_t>(
                        random.bounded(config_.memoryRows)),
                    base.second};
            }
        }
        else
        {
            address = randomAddress();
        }

        const std::uint64_t key = addressKey(
            address.first, address.second, config_.memoryColumns);
        if (!used.insert(key).second)
        {
            // A deterministic scan guarantees termination for dense maps.
            const std::uint64_t start = random.bounded(cellCount);
            bool found = false;
            for (std::uint64_t offset = 0;
                 offset < cellCount; ++offset)
            {
                const std::uint64_t candidate = (start + offset) % cellCount;
                if (used.insert(candidate).second)
                {
                    address = {
                        static_cast<std::uint32_t>(
                            candidate / config_.memoryColumns),
                        static_cast<std::uint32_t>(
                            candidate % config_.memoryColumns)};
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                throw std::logic_error(
                    "Unable to find a unique fault address");
            }
        }
        addresses.push_back(address);
    }

    std::vector<Fault> faults;
    faults.reserve(addresses.size());
    for (const auto &address : addresses)
    {
        Fault fault{};
        fault.r = static_cast<int>(address.first);
        fault.c = static_cast<int>(address.second);
        fault.HBMID = 0;
        fault.ChannelID = 0;
        fault.BankID = static_cast<int>(
            runIndex % static_cast<std::size_t>(
                std::numeric_limits<int>::max()));
        fault.SubarrayGroupID = 0;
        fault.SubarrayID = static_cast<int>(subarray);
        faults.push_back(fault);
    }
    return faults;
}

FaultGroup DynamicFaultGenerator::generate(std::size_t runIndex)
{
    StableRandom random(
        config_.randomSeed ^
        (0xd1b54a32d192ed03ULL *
         (static_cast<std::uint64_t>(runIndex) + 1)));
    const std::vector<std::size_t> counts = countsForGroup();
    FaultGroup group;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        group[subarray] = faultsForSubarray(
            counts[subarray], runIndex, subarray, random);
    }
    return group;
}

std::vector<FaultGroup> DynamicFaultGenerator::generateRuns(
    std::size_t runCount)
{
    std::vector<FaultGroup> groups;
    groups.reserve(runCount);
    for (std::size_t runIndex = 0;
         runIndex < runCount; ++runIndex)
    {
        groups.push_back(generate(runIndex));
    }
    return groups;
}

} // namespace dynamic_spare
