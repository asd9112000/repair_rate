#include "DirectionalMultiConfigAnalyzer.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(bool value, const std::string &message)
{
    if (!value)
        throw std::runtime_error(message);
}

Fault fault(int row, int column)
{
    Fault value{};
    value.HBMID = 0;
    value.ChannelID = 0;
    value.BankID = 0;
    value.SubarrayGroupID = 0;
    value.SubarrayID = 0;
    value.r = row;
    value.c = column;
    return value;
}

const dynamic_spare::DirectionalConfigAnalysis &config(
    const dynamic_spare::DirectionalMultiConfigAnalysis &analysis,
    dynamic_spare::DirectionalConfigId id)
{
    return analysis.configs[static_cast<std::size_t>(id)];
}

void testPrefixAndOverflow()
{
    std::vector<Fault> faults{
        fault(0, 0), fault(1, 1), fault(2, 2), fault(3, 3), fault(4, 4)};
    const auto result = dynamic_spare::DirectionalMultiConfigAnalyzer{}.analyze(
        faults, 0);
    require(result.sharedPivotOccupancy == 5,
            "five independent pivots did not fill the shared payload");
    require(config(result, dynamic_spare::DirectionalConfigId::Cfg1_2R1C)
                    .addressActive == std::array<bool, 5>{{true, true, true, false, false}},
            "2R1C did not see the first three shared pivots");
    require(config(result, dynamic_spare::DirectionalConfigId::Cfg0_2R2C)
                    .addressActive == std::array<bool, 5>{{true, true, true, true, false}},
            "2R2C did not see the first four shared pivots");
    require(config(result, dynamic_spare::DirectionalConfigId::Cfg2_3R2C)
                    .addressActive == std::array<bool, 5>{{true, true, true, true, true}},
            "3R2C did not see the first five shared pivots");
    require(config(result, dynamic_spare::DirectionalConfigId::Cfg1_2R1C)
                    .camReuseTemporaryFaults.size() == 2,
            "post-full pivots were not isolated for 2R1C");
    require(config(result, dynamic_spare::DirectionalConfigId::Cfg0_2R2C)
                    .camReuseTemporaryFaults.size() == 1,
            "fourth pivot was not retained as a 2R2C address pivot");
    require(config(result, dynamic_spare::DirectionalConfigId::Cfg2_3R2C)
                    .camReuseTemporaryFaults.empty(),
            "fifth pivot incorrectly overflowed 3R2C");
}

void testMustAndPatternContract()
{
    // Two faults in one row distinguish Cs=1 from Cs=2.
    const auto result = dynamic_spare::DirectionalMultiConfigAnalyzer{}.analyze(
        std::vector<Fault>{fault(7, 1), fault(7, 2)}, 0);
    const auto &small = config(result, dynamic_spare::DirectionalConfigId::Cfg1_2R1C);
    const auto &base = config(result, dynamic_spare::DirectionalConfigId::Cfg0_2R2C);
    require(small.lowestValidPatternId != 0 && base.lowestValidPatternId != 0,
            "valid configurations must retain a one-based PatternID");
    require(small.lowestValidPatternId <= 3 && base.lowestValidPatternId <= 6,
            "PatternID does not match its configuration candidate count");
    require(!small.reconstructedMappings.empty(),
            "PatternID did not reconstruct conventional repair lines");
    require(std::any_of(small.reconstructedMappings.begin(),
                        small.reconstructedMappings.end(),
                        [](const dynamic_spare::RepairLineMapping &mapping)
                        { return mapping.dimension == dynamic_spare::SpareDimension::Row && mapping.originalAddress == 7; }),
            "2R1C RowMust did not force the shared-count row repair");
    require(base.lowestValidPatternId != 0,
            "2R2C should remain feasible when the same row is not mandatory");
}

void testTransposeDescriptors()
{
    const auto &descriptors = dynamic_spare::DirectionalMultiConfigAnalyzer::descriptors();
    require(descriptors[4].transposeCanonicalView &&
                descriptors[5].transposeCanonicalView &&
                descriptors[6].transposeCanonicalView,
            "physical transpose configurations lost their canonical tags");
    require(descriptors[4].rows == 1 && descriptors[4].columns == 2 &&
                descriptors[5].rows == 2 && descriptors[5].columns == 3 &&
                descriptors[6].rows == 1 && descriptors[6].columns == 3,
            "physical ConfigID R/C envelopes changed");
}

void testGroupPoliciesAndPendingBuffer()
{
    std::array<std::vector<Fault>, dynamic_spare::kSubarrayCount> faults;
    for (std::size_t subarray = 0; subarray < faults.size(); ++subarray)
    {
        Fault value = fault(static_cast<int>(subarray), static_cast<int>(subarray));
        value.SubarrayID = static_cast<int>(subarray);
        faults[subarray].push_back(value);
    }
    dynamic_spare::SimulationConfig ledgerConfig;
    ledgerConfig.topology = dynamic_spare::SharingTopology::Directional;
    ledgerConfig.sharedRows = 1;
    ledgerConfig.sharedColumns = 1;
    ledgerConfig.modifiers.maximumGroupBorrowedSpares = 1;
    const dynamic_spare::DirectionalMultiConfigAnalyzer analyzer;
    const auto early = analyzer.analyzeGroup(
        faults, ledgerConfig, dynamic_spare::DirectionalSolutionTakePolicy::Early);
    const auto group = analyzer.analyzeGroup(
        faults, ledgerConfig, dynamic_spare::DirectionalSolutionTakePolicy::Group);
    require(!early.success || group.success,
            "GROUP must dominate EARLY under identical directional rules");
    require(group.earlySuccess == early.success && group.groupSuccess == group.success,
            "GROUP result did not report both policy outcomes");
    require(group.groupCombinationsChecked != 0,
            "GROUP did not search ConfigID combinations");
    if (group.success)
    {
        require(group.pendingRepairBuffer.size() == 4,
                "successful GROUP selection did not stage four pending records");
        for (const auto &record : group.pendingRepairBuffer)
            require(record.patternId != 0,
                    "PendingRepairBuffer retained an invalid PatternID");
    }
}

void emitRtlGolden(const std::string &path)
{
    std::ofstream output(path);
    require(static_cast<bool>(output), "cannot create RTL golden vector file");
    dynamic_spare::SimulationConfig ledgerConfig;
    ledgerConfig.topology = dynamic_spare::SharingTopology::Directional;
    ledgerConfig.sharedRows = 1;
    ledgerConfig.sharedColumns = 1;
    ledgerConfig.modifiers.maximumGroupBorrowedSpares = 3;
    const dynamic_spare::DirectionalMultiConfigAnalyzer analyzer;
    const auto emitCase = [&](const std::string &name,
                              dynamic_spare::DirectionalSolutionTakePolicy policy,
                              std::array<std::vector<Fault>, dynamic_spare::kSubarrayCount> faults)
    {
        const auto result = analyzer.analyzeGroup(faults, ledgerConfig, policy);
        output << "CASE " << name << ' '
               << (policy == dynamic_spare::DirectionalSolutionTakePolicy::Group) << ' '
               << result.success << '\n';
        for (std::size_t sa = 0; sa < faults.size(); ++sa)
        {
            output << "SA " << sa << ' ' << faults[sa].size();
            for (const Fault &value : faults[sa])
                output << ' ' << value.r << ' ' << value.c;
            for (std::uint8_t pattern : result.configPatternMap[sa])
                output << ' ' << static_cast<unsigned>(pattern);
            output << '\n';
        }
        if (result.success)
        {
            for (const auto &record : result.pendingRepairBuffer)
            {
                output << "SEL " << record.subarrayId << ' '
                       << static_cast<std::size_t>(record.configId) << ' '
                       << static_cast<unsigned>(record.patternId);
                std::vector<int> rows;
                std::vector<int> columns;
                for (const auto &mapping : record.conventionalMappings)
                    (mapping.dimension == dynamic_spare::SpareDimension::Row ? rows : columns)
                        .push_back(mapping.originalAddress);
                output << ' ' << rows.size();
                for (int address : rows) output << ' ' << address;
                output << ' ' << columns.size();
                for (int address : columns) output << ' ' << address;
                output << '\n';
            }
        }
        output << "END\n";
    };
    std::array<std::vector<Fault>, dynamic_spare::kSubarrayCount> single;
    for (std::size_t sa = 0; sa < single.size(); ++sa)
    {
        Fault value = fault(static_cast<int>(10 + sa), static_cast<int>(20 + sa));
        value.SubarrayID = static_cast<int>(sa);
        single[sa].push_back(value);
    }
    emitCase("single_group", dynamic_spare::DirectionalSolutionTakePolicy::Group, single);
    emitCase("single_early", dynamic_spare::DirectionalSolutionTakePolicy::Early, single);
    auto mixed = single;
    mixed[0].clear(); mixed[1].clear();
    for (int column : {1,2}) { Fault value=fault(7,column); value.SubarrayID=0; mixed[0].push_back(value); }
    for (int row : {3,4}) { Fault value=fault(row,9); value.SubarrayID=1; mixed[1].push_back(value); }
    emitCase("must_hybrid_group", dynamic_spare::DirectionalSolutionTakePolicy::Group, mixed);
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        if (argc == 3 && std::string(argv[1]) == "--emit-rtl-golden")
        {
            emitRtlGolden(argv[2]);
            std::cout << "directional RTL golden vectors written\n";
            return 0;
        }
        testPrefixAndOverflow();
        testMustAndPatternContract();
        testTransposeDescriptors();
        testGroupPoliciesAndPendingBuffer();
    }
    catch (const std::exception &error)
    {
        throw std::runtime_error(std::string("directional multi-config test: ") + error.what());
    }
    return 0;
}
