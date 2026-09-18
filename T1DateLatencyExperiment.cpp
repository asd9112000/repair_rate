#include "BistOverlapTimingModel.hpp"
#include "DssBistFaultTimeline.hpp"
#include "DynamicFaultGenerator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr std::uint64_t kDateSeed = 20260914ULL;
constexpr std::size_t kFaultsPerGroup = 28;
constexpr std::size_t kFaultsPerSa = 7;
constexpr const char *kScenarioId = "E0L_DATE_2X2_LATENCY_V1";

struct SampleRecord
{
    std::size_t sampleId = 0;
    std::array<std::size_t, dynamic_spare::kSubarrayCount> faultCounts{};
    std::array<std::optional<std::uint64_t>, dynamic_spare::kSubarrayCount>
        lastFaultAccept{};
    std::array<std::optional<Fault>, dynamic_spare::kSubarrayCount>
        lastFault{};
    dynamic_spare::BistOverlapTimingResult timing;
    std::size_t criticalSa = 0;
};

struct ScalarStatistics
{
    double mean = 0.0;
    std::uint64_t median = 0;
    std::uint64_t p5 = 0;
    std::uint64_t p90 = 0;
    std::uint64_t p95 = 0;
    std::uint64_t p99 = 0;
    std::uint64_t minimum = 0;
    std::uint64_t maximum = 0;
};

struct SignedScalarStatistics
{
    double mean = 0.0;
    std::int64_t median = 0;
    std::int64_t p5 = 0;
    std::int64_t p95 = 0;
    std::int64_t minimum = 0;
    std::int64_t maximum = 0;
};

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

dynamic_spare::SimulationConfig frozenDateConfig()
{
    // This intentionally matches tests/e0l_generate_candidate_corpus.cpp and
    // the S1D E0L_DATE_2X2_LATENCY_V1 corpus recipe without adding a new
    // latency-only distribution.
    dynamic_spare::SimulationConfig config;
    config.spareRows = 2;
    config.spareColumns = 2;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.dataWidthBits = 256;
    config.rowAddressWidthBits = 9;
    config.columnAddressWidthBits = 13;
    config.faultCount = kFaultsPerGroup;
    config.randomSeed = kDateSeed;
    config.memoryRows = 512;
    config.memoryColumns = 8192;
    config.faultCountModel = dynamic_spare::FaultCountModel::Uniform;
    config.faultSpatialModel = dynamic_spare::FaultSpatialModel::Mixed;
    config.layout = dynamic_spare::GroupLayout::Grid2x2;
    config.topology = dynamic_spare::SharingTopology::Directional;
    config.modifiers.maximumGroupBorrowedSpares = 1;
    return config;
}

std::string optionalNumber(const std::optional<std::uint64_t> &value)
{
    return value ? std::to_string(*value) : "N/A";
}

std::string optionalFaultField(
    const std::optional<Fault> &fault, bool row)
{
    if (!fault)
    {
        return "N/A";
    }
    return std::to_string(row ? fault->r : fault->c);
}

std::uint64_t percentile(
    const std::vector<std::uint64_t> &sortedValues, unsigned int percent)
{
    require(!sortedValues.empty(), "Cannot calculate a percentile of no samples");
    require(percent <= 100, "Invalid percentile");
    // Nearest-rank percentile: p95 is element ceil(0.95*N), one-based.
    const std::size_t index = percent == 0
        ? 0
        : ((static_cast<std::size_t>(percent) * sortedValues.size() + 99) / 100) - 1;
    return sortedValues[std::min(index, sortedValues.size() - 1)];
}

ScalarStatistics calculateStatistics(std::vector<std::uint64_t> values)
{
    require(!values.empty(), "Cannot summarize no samples");
    std::sort(values.begin(), values.end());
    const long double sum = std::accumulate(
        values.begin(), values.end(), static_cast<long double>(0));
    ScalarStatistics statistics;
    statistics.mean = static_cast<double>(sum / values.size());
    statistics.median = percentile(values, 50);
    statistics.p5 = percentile(values, 5);
    statistics.p90 = percentile(values, 90);
    statistics.p95 = percentile(values, 95);
    statistics.p99 = percentile(values, 99);
    statistics.minimum = values.front();
    statistics.maximum = values.back();
    return statistics;
}

std::int64_t signedPercentile(
    const std::vector<std::int64_t> &sortedValues, unsigned int percent)
{
    require(!sortedValues.empty(), "Cannot calculate a percentile of no samples");
    require(percent <= 100, "Invalid percentile");
    const std::size_t index = percent == 0
        ? 0
        : ((static_cast<std::size_t>(percent) * sortedValues.size() + 99) / 100) - 1;
    return sortedValues[std::min(index, sortedValues.size() - 1)];
}

SignedScalarStatistics calculateSignedStatistics(std::vector<std::int64_t> values)
{
    require(!values.empty(), "Cannot summarize no samples");
    std::sort(values.begin(), values.end());
    const long double sum = std::accumulate(
        values.begin(), values.end(), static_cast<long double>(0));
    SignedScalarStatistics statistics;
    statistics.mean = static_cast<double>(sum / values.size());
    statistics.median = signedPercentile(values, 50);
    statistics.p5 = signedPercentile(values, 5);
    statistics.p95 = signedPercentile(values, 95);
    statistics.minimum = values.front();
    statistics.maximum = values.back();
    return statistics;
}

std::optional<double> pearsonCorrelation(
    const std::vector<std::uint64_t> &x, const std::vector<std::uint64_t> &y)
{
    require(x.size() == y.size(), "Correlation vectors differ in length");
    if (x.empty())
    {
        return std::nullopt;
    }
    long double meanX = 0;
    long double meanY = 0;
    for (std::size_t index = 0; index < x.size(); ++index)
    {
        meanX += x[index];
        meanY += y[index];
    }
    meanX /= x.size();
    meanY /= y.size();
    long double numerator = 0;
    long double xSquared = 0;
    long double ySquared = 0;
    for (std::size_t index = 0; index < x.size(); ++index)
    {
        const long double dx = x[index] - meanX;
        const long double dy = y[index] - meanY;
        numerator += dx * dy;
        xSquared += dx * dx;
        ySquared += dy * dy;
    }
    if (xSquared == 0 || ySquared == 0)
    {
        return std::nullopt;
    }
    return static_cast<double>(numerator / std::sqrt(xSquared * ySquared));
}

void writeRawCsv(
    const std::filesystem::path &path, const std::vector<SampleRecord> &records)
{
    std::ofstream output(path);
    require(output.good(), "Unable to create raw T1 DATE CSV");
    output << "sample_id,seed,scenario_id,fault_count_A,fault_count_B,"
              "fault_count_C,fault_count_D,fault_count_group,"
              "last_fault_accept_A,last_fault_accept_B,last_fault_accept_C,"
              "last_fault_accept_D,candidate_ready_A,candidate_ready_B,"
              "candidate_ready_C,candidate_ready_D,candidate_catchup_A,"
              "candidate_catchup_B,candidate_catchup_C,candidate_catchup_D,"
              "ownership_wait_A,ownership_wait_B,ownership_wait_C,"
              "ownership_wait_D,latest_group_candidate_ready,group_test_done,"
              "group_hidden_analysis_slack,group_provisional_post_bist_latency,"
              "all_candidates_ready_before_test_done,critical_sa,"
              "last_fault_row_A,last_fault_column_A,last_fault_row_B,"
              "last_fault_column_B,last_fault_row_C,last_fault_column_C,"
              "last_fault_row_D,last_fault_column_D\n";
    for (const SampleRecord &record : records)
    {
        const auto &sa = record.timing.subarrays;
        output << record.sampleId << ',' << kDateSeed << ',' << kScenarioId;
        for (std::size_t index = 0; index < dynamic_spare::kSubarrayCount; ++index)
        {
            output << ',' << record.faultCounts[index];
        }
        output << ',' << kFaultsPerGroup;
        for (std::size_t index = 0; index < dynamic_spare::kSubarrayCount; ++index)
        {
            output << ',' << optionalNumber(record.lastFaultAccept[index]);
        }
        for (std::size_t index = 0; index < dynamic_spare::kSubarrayCount; ++index)
        {
            output << ',' << sa[index].candidateReadyCycle;
        }
        for (std::size_t index = 0; index < dynamic_spare::kSubarrayCount; ++index)
        {
            output << ',' << optionalNumber(sa[index].candidateCatchupLatency);
        }
        for (std::size_t index = 0; index < dynamic_spare::kSubarrayCount; ++index)
        {
            output << ',' << optionalNumber(sa[index].ownershipWaitCycles);
        }
        output << ',' << record.timing.group.latestCandidateReadyCycle
               << ',' << record.timing.group.groupTestDoneCycle
               << ',' << record.timing.group.hiddenAnalysisSlack
               << ',' << record.timing.group.provisionalPostBistLatency
               << ',' << (record.timing.group.allCandidatesReadyBeforeTestDone ? "true" : "false")
               << ',' << static_cast<char>('A' + record.criticalSa);
        for (std::size_t index = 0; index < dynamic_spare::kSubarrayCount; ++index)
        {
            output << ',' << optionalFaultField(record.lastFault[index], true)
                   << ',' << optionalFaultField(record.lastFault[index], false);
        }
        output << '\n';
    }
}

void writeCdfAndHistogram(
    const std::filesystem::path &cdfPath,
    const std::filesystem::path &histogramPath,
    const std::vector<SampleRecord> &records)
{
    std::map<std::uint64_t, std::size_t> exactCounts;
    for (const SampleRecord &record : records)
    {
        ++exactCounts[record.timing.group.provisionalPostBistLatency];
    }

    std::ofstream cdf(cdfPath);
    std::ofstream histogram(histogramPath);
    require(cdf.good() && histogram.good(), "Unable to create CDF/histogram CSV");
    cdf << "scenario,latency_cycles,cdf,ccdf,count\n";
    histogram << "scenario,bin_label,bin_lower_cycles,bin_upper_cycles,count,percent\n";
    std::size_t cumulative = 0;
    for (const auto &entry : exactCounts)
    {
        cumulative += entry.second;
        const double cdfValue = static_cast<double>(cumulative) / records.size();
        cdf << kScenarioId << ',' << entry.first << ',' << std::fixed
            << std::setprecision(9) << cdfValue << ',' << (1.0 - cdfValue)
            << ',' << entry.second << '\n';
    }

    const std::array<std::pair<std::uint64_t, std::uint64_t>, 7> bins{{
        {0, 0}, {1, 1}, {2, 2}, {3, 4}, {5, 8}, {9, 16}, {17, 32}}};
    std::size_t accounted = 0;
    for (const auto &bin : bins)
    {
        std::size_t count = 0;
        for (const auto &entry : exactCounts)
        {
            if (entry.first >= bin.first && entry.first <= bin.second)
            {
                count += entry.second;
            }
        }
        accounted += count;
        const std::string label = bin.first == bin.second
            ? std::to_string(bin.first)
            : std::to_string(bin.first) + "-" + std::to_string(bin.second);
        histogram << kScenarioId << ',' << label << ',' << bin.first << ','
                  << bin.second << ',' << count << ',' << std::fixed
                  << std::setprecision(6)
                  << (100.0 * static_cast<double>(count) / records.size()) << '\n';
    }
    if (accounted < records.size())
    {
        std::size_t overflow = records.size() - accounted;
        histogram << kScenarioId << ",33+,33,INF," << overflow << ','
                  << std::fixed << std::setprecision(6)
                  << (100.0 * static_cast<double>(overflow) / records.size()) << '\n';
    }
}

void writeWorstExamples(
    const std::filesystem::path &path, std::vector<SampleRecord> records)
{
    std::sort(records.begin(), records.end(), [](const SampleRecord &left,
                                                  const SampleRecord &right) {
        const std::uint64_t lhs = left.timing.group.provisionalPostBistLatency;
        const std::uint64_t rhs = right.timing.group.provisionalPostBistLatency;
        return lhs != rhs ? lhs > rhs : left.sampleId < right.sampleId;
    });
    std::ofstream output(path);
    require(output.good(), "Unable to create worst-example CSV");
    output << "rank,sample_id,exposed_latency,hidden_slack,critical_sa,"
              "fault_count_A,fault_count_B,fault_count_C,fault_count_D,"
              "last_fault_accept_A,last_fault_accept_B,last_fault_accept_C,"
              "last_fault_accept_D,candidate_ready_A,candidate_ready_B,"
              "candidate_ready_C,candidate_ready_D,ownership_wait_A,"
              "ownership_wait_B,ownership_wait_C,ownership_wait_D,"
              "critical_last_fault_row,critical_last_fault_column\n";
    const std::size_t count = std::min<std::size_t>(10, records.size());
    for (std::size_t index = 0; index < count; ++index)
    {
        const SampleRecord &record = records[index];
        const auto &sa = record.timing.subarrays;
        output << (index + 1) << ',' << record.sampleId << ','
               << record.timing.group.provisionalPostBistLatency << ','
               << record.timing.group.hiddenAnalysisSlack << ','
               << static_cast<char>('A' + record.criticalSa);
        for (std::size_t subarray = 0; subarray < dynamic_spare::kSubarrayCount; ++subarray)
        {
            output << ',' << record.faultCounts[subarray];
        }
        for (std::size_t subarray = 0; subarray < dynamic_spare::kSubarrayCount; ++subarray)
        {
            output << ',' << optionalNumber(record.lastFaultAccept[subarray]);
        }
        for (std::size_t subarray = 0; subarray < dynamic_spare::kSubarrayCount; ++subarray)
        {
            output << ',' << sa[subarray].candidateReadyCycle;
        }
        for (std::size_t subarray = 0; subarray < dynamic_spare::kSubarrayCount; ++subarray)
        {
            output << ',' << optionalNumber(sa[subarray].ownershipWaitCycles);
        }
        output << ',' << optionalFaultField(record.lastFault[record.criticalSa], true)
               << ',' << optionalFaultField(record.lastFault[record.criticalSa], false)
               << '\n';
    }
}

void writeSummary(
    const std::filesystem::path &summaryPath,
    const std::filesystem::path &perSaPath,
    const std::filesystem::path &criticalPath,
    const std::filesystem::path &correlationPath,
    const std::filesystem::path &tablePath,
    const std::filesystem::path &metadataPath,
    const std::vector<SampleRecord> &records,
    std::size_t generated,
    std::size_t outOfEnvelope)
{
    require(!records.empty(), "No valid groups are available for DATE summary");
    std::vector<std::uint64_t> latency;
    std::vector<std::int64_t> hiddenSlack;
    std::array<std::vector<std::uint64_t>, dynamic_spare::kSubarrayCount> catchup;
    std::array<std::vector<std::uint64_t>, dynamic_spare::kSubarrayCount> ownershipWait;
    std::array<std::size_t, dynamic_spare::kSubarrayCount> criticalCounts{};
    std::vector<std::uint64_t> lastFaultCycle;
    std::vector<std::uint64_t> groupFaultCount;
    std::size_t fullyHidden = 0;

    for (const SampleRecord &record : records)
    {
        const auto &group = record.timing.group;
        require(group.latestCandidateReadyCycle == std::max(
                    std::max(record.timing.subarrays[0].candidateReadyCycle,
                             record.timing.subarrays[1].candidateReadyCycle),
                    std::max(record.timing.subarrays[2].candidateReadyCycle,
                             record.timing.subarrays[3].candidateReadyCycle)),
                "Group candidate-ready maximum sanity check failed");
        require(group.provisionalPostBistLatency ==
                    (group.latestCandidateReadyCycle > group.groupTestDoneCycle
                     ? group.latestCandidateReadyCycle - group.groupTestDoneCycle : 0),
                "Primary latency formula sanity check failed");
        require(group.allCandidatesReadyBeforeTestDone ==
                    (group.provisionalPostBistLatency == 0),
                "Fully-hidden identity sanity check failed");
        latency.push_back(group.provisionalPostBistLatency);
        hiddenSlack.push_back(group.hiddenAnalysisSlack);
        fullyHidden += group.allCandidatesReadyBeforeTestDone ? 1 : 0;
        ++criticalCounts[record.criticalSa];
        std::uint64_t latestFault = 0;
        for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
        {
            require(record.timing.subarrays[sa].candidateCatchupLatency.has_value() &&
                        record.timing.subarrays[sa].ownershipWaitCycles.has_value(),
                    "Baseline's positive-fault SA lacks catch-up timing");
            catchup[sa].push_back(*record.timing.subarrays[sa].candidateCatchupLatency);
            ownershipWait[sa].push_back(*record.timing.subarrays[sa].ownershipWaitCycles);
            require(record.lastFaultAccept[sa].has_value(),
                    "Baseline's positive-fault SA lacks last acceptance cycle");
            latestFault = std::max(latestFault, *record.lastFaultAccept[sa]);
        }
        lastFaultCycle.push_back(latestFault);
        groupFaultCount.push_back(kFaultsPerGroup);
    }

    const ScalarStatistics exposed = calculateStatistics(latency);
    const SignedScalarStatistics slack = calculateSignedStatistics(hiddenSlack);
    const auto arrivalCorrelation = pearsonCorrelation(lastFaultCycle, latency);
    const auto faultCountCorrelation = pearsonCorrelation(groupFaultCount, latency);
    std::ofstream summary(summaryPath);
    std::ofstream perSa(perSaPath);
    std::ofstream critical(criticalPath);
    std::ofstream correlation(correlationPath);
    std::ofstream table(tablePath);
    std::ofstream metadata(metadataPath);
    require(summary.good() && perSa.good() && critical.good() && correlation.good() &&
                table.good() && metadata.good(),
            "Unable to create DATE T1 summary artifacts");

    summary << "scenario,generated_groups,valid_groups,out_of_t1_envelope_groups,"
               "out_of_t1_envelope_percent,fully_hidden_count,fully_hidden_percent,"
               "exposed_gt0_count,exposed_gt0_percent,mean_exposed_latency_cycles,"
               "median_exposed_latency_cycles,p90_exposed_latency_cycles,"
               "p95_exposed_latency_cycles,p99_exposed_latency_cycles,"
               "max_exposed_latency_cycles,mean_hidden_slack_cycles,"
               "median_hidden_slack_cycles,p5_hidden_slack_cycles,"
               "p95_hidden_slack_cycles,min_hidden_slack_cycles,max_hidden_slack_cycles,"
               "p_latency_eq_0,p_latency_le_1,p_latency_le_2,p_latency_le_4,"
               "p_latency_le_8,p_latency_le_16,p_latency_le_32,"
               "normalized_mean_percent,normalized_p95_percent,"
               "normalized_p99_percent,normalized_max_percent\n";
    const auto probability = [&latency](std::uint64_t threshold) {
        return static_cast<double>(std::count_if(
                   latency.begin(), latency.end(),
                   [threshold](std::uint64_t value) { return value <= threshold; })) /
               latency.size();
    };
    const std::size_t exposedCount = latency.size() - fullyHidden;
    summary << kScenarioId << ',' << generated << ',' << records.size() << ','
            << outOfEnvelope << ',' << std::fixed << std::setprecision(6)
            << (100.0 * outOfEnvelope / generated) << ',' << fullyHidden << ','
            << (100.0 * fullyHidden / records.size()) << ',' << exposedCount << ','
            << (100.0 * exposedCount / records.size()) << ',' << std::setprecision(6)
            << exposed.mean << ',' << exposed.median << ',' << exposed.p90 << ','
            << exposed.p95 << ',' << exposed.p99 << ',' << exposed.maximum << ','
            << slack.mean << ',' << slack.median << ',' << slack.p5 << ','
            << slack.p95 << ',' << slack.minimum << ',' << slack.maximum;
    for (const std::uint64_t threshold : {0ULL, 1ULL, 2ULL, 4ULL, 8ULL, 16ULL, 32ULL})
    {
        summary << ',' << probability(threshold);
    }
    summary << ',' << (100.0 * exposed.mean / 65536.0) << ','
            << (100.0 * exposed.p95 / 65536.0) << ','
            << (100.0 * exposed.p99 / 65536.0) << ','
            << (100.0 * exposed.maximum / 65536.0) << '\n';

    perSa << "scenario,sa,mean_catchup_cycles,median_catchup_cycles,"
             "p95_catchup_cycles,p99_catchup_cycles,max_catchup_cycles,"
             "mean_ownership_wait_cycles\n";
    for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
    {
        const ScalarStatistics stats = calculateStatistics(catchup[sa]);
        const long double waitSum = std::accumulate(
            ownershipWait[sa].begin(), ownershipWait[sa].end(),
            static_cast<long double>(0));
        perSa << kScenarioId << ',' << static_cast<char>('A' + sa) << ','
              << std::fixed << std::setprecision(6) << stats.mean << ','
              << stats.median << ',' << stats.p95 << ',' << stats.p99 << ','
              << stats.maximum << ','
              << static_cast<double>(waitSum / ownershipWait[sa].size()) << '\n';
    }

    critical << "scenario,critical_sa,count,percent\n";
    for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
    {
        critical << kScenarioId << ',' << static_cast<char>('A' + sa) << ','
                 << criticalCounts[sa] << ',' << std::fixed << std::setprecision(6)
                 << (100.0 * criticalCounts[sa] / records.size()) << '\n';
    }

    correlation << "scenario,analysis,metric,status,value,note\n";
    correlation << kScenarioId << ",pearson,last_fault_accept_cycle_vs_exposed_latency,"
                << (arrivalCorrelation ? "DEFINED" : "N_A") << ','
                << (arrivalCorrelation ? std::to_string(*arrivalCorrelation) : "N/A")
                << ",descriptive_only_no_causality_claim\n";
    correlation << kScenarioId << ",pearson,fault_count_group_vs_exposed_latency,"
                << (faultCountCorrelation ? "DEFINED" : "N_A") << ','
                << (faultCountCorrelation ? std::to_string(*faultCountCorrelation) : "N/A")
                << ",fault_count_is_constant_at_28_in_the_frozen_DATE_baseline\n";
    std::vector<std::size_t> order(records.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&lastFaultCycle](std::size_t left,
                                                            std::size_t right) {
        return lastFaultCycle[left] < lastFaultCycle[right];
    });
    for (std::size_t quartile = 0; quartile < 4; ++quartile)
    {
        const std::size_t first = quartile * order.size() / 4;
        const std::size_t last = (quartile + 1) * order.size() / 4;
        std::uint64_t minArrival = std::numeric_limits<std::uint64_t>::max();
        std::uint64_t maxArrival = 0;
        long double latencySum = 0;
        for (std::size_t index = first; index < last; ++index)
        {
            minArrival = std::min(minArrival, lastFaultCycle[order[index]]);
            maxArrival = std::max(maxArrival, lastFaultCycle[order[index]]);
            latencySum += latency[order[index]];
        }
        correlation << kScenarioId << ",stratified,last_fault_arrival_quartile_"
                    << (quartile + 1) << ",DEFINED,"
                    << static_cast<double>(latencySum / (last - first))
                    << ",count=" << (last - first) << ";arrival_range="
                    << minArrival << '-' << maxArrival << '\n';
    }

    table << "| Scenario | Fault parameters | Groups | Fully hidden | Mean exposed | Median | P95 | P99 | Max | Mean hidden slack |\n"
             "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n"
          << "| " << kScenarioId
          << " | Uniform; 28 fixed/group; 7/SA; Mixed; seed 20260914 | "
          << records.size() << " | " << std::fixed << std::setprecision(3)
          << (100.0 * fullyHidden / records.size()) << "% | " << exposed.mean
          << " | " << exposed.median << " | " << exposed.p95 << " | "
          << exposed.p99 << " | " << exposed.maximum << " | " << slack.mean
          << " |\n";

    metadata << "scenario_id=" << kScenarioId << '\n'
             << "master_seed=" << kDateSeed << '\n'
             << "scenario_seed=" << kDateSeed << '\n'
             << "generator=DynamicFaultGenerator\n"
             << "generator_source=src/DynamicFaultGenerator.cpp\n"
             << "frozen_config_source=tests/e0l_generate_candidate_corpus.cpp\n"
             << "fault_count_model=Uniform\n"
             << "fault_count_group=28_fixed\n"
             << "faults_per_sa=7_fixed\n"
             << "fault_spatial_model=Mixed\n"
             << "mixed_cluster_probability=0.20\n"
             << "mixed_same_line_effective_probability=0.30\n"
             << "memory_rows=512\n"
             << "memory_physical_cell_columns=8192\n"
             << "bist_word_bits=256\n"
             << "serial_bist_completion_cycle=65536\n"
             << "generated_groups=" << generated << '\n'
             << "valid_groups=" << records.size() << '\n'
             << "out_of_t1_timing_envelope_groups=" << outOfEnvelope << '\n'
             << "envelope=MAX_FAULTS_PER_SA=12\n";
}

SampleRecord evaluateSample(
    std::size_t sampleId,
    const dynamic_spare::FaultGroup &faults,
    const dynamic_spare::SerialBistSchedule &schedule,
    const dynamic_spare::BistOverlapTimingModel &model)
{
    SampleRecord record;
    record.sampleId = sampleId;
    const dynamic_spare::DssBistFaultTimeline timeline =
        dynamic_spare::materializeDssBistFaultTimeline(faults, schedule);
    for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
    {
        record.faultCounts[sa] = faults[sa].size();
    }
    for (const dynamic_spare::AcceptedFaultEvent &event : timeline.acceptedFaults)
    {
        if (!record.lastFaultAccept[event.subarrayId] ||
            event.acceptCycle >= *record.lastFaultAccept[event.subarrayId])
        {
            record.lastFaultAccept[event.subarrayId] = event.acceptCycle;
            record.lastFault[event.subarrayId] = event.fault;
        }
    }
    record.timing = model.evaluateGroup(faults, schedule);
    record.criticalSa = 0;
    for (std::size_t sa = 1; sa < dynamic_spare::kSubarrayCount; ++sa)
    {
        if (record.timing.subarrays[sa].candidateReadyCycle >
            record.timing.subarrays[record.criticalSa].candidateReadyCycle)
        {
            // A deterministic A->B->C->D first-max rule resolves a rare tie.
            record.criticalSa = sa;
        }
    }
    return record;
}

void usage()
{
    std::cerr << "usage: T1DateLatencyExperiment <output_root> <stage_a|formal> <groups>\n";
}

} // namespace

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 4)
        {
            usage();
            return 1;
        }
        const std::filesystem::path root(argv[1]);
        const std::string stage(argv[2]);
        const std::size_t groupCount = static_cast<std::size_t>(std::stoull(argv[3]));
        if ((stage != "stage_a" && stage != "formal" &&
             stage != "formal_1m") || groupCount == 0)
        {
            usage();
            return 1;
        }
        std::filesystem::create_directories(root / "raw");
        std::filesystem::create_directories(root / "summary");
        std::filesystem::create_directories(root / "tables");
        std::filesystem::create_directories(root / "cdf");
        std::filesystem::create_directories(root / "logs");

        const dynamic_spare::SimulationConfig config = frozenDateConfig();
        const dynamic_spare::SerialBistSchedule schedule;
        require(schedule.geometry.rows == config.memoryRows &&
                    schedule.geometry.cellColumns == config.memoryColumns &&
                    schedule.geometry.wordBits == config.dataWidthBits &&
                    schedule.cyclesPerWord == 1 && schedule.completionCycle() == 65536,
                "Frozen DATE serial-BIST geometry no longer matches the T1 contract");
        dynamic_spare::DynamicFaultGenerator generator(config);
        const dynamic_spare::BistOverlapTimingModel model;
        std::ofstream outOfEnvelopeCsv(
            root / "logs" / (stage + "_out_of_t1_envelope.csv"));
        require(outOfEnvelopeCsv.good(),
                "Unable to create out-of-envelope classification CSV");
        outOfEnvelopeCsv << "sample_id,seed,scenario_id,fault_count_A,"
                            "fault_count_B,fault_count_C,fault_count_D,"
                            "classification\n";
        std::vector<SampleRecord> records;
        records.reserve(groupCount);
        std::size_t outOfEnvelope = 0;
        for (std::size_t sampleId = 0; sampleId < groupCount; ++sampleId)
        {
            const dynamic_spare::FaultGroup faults = generator.generate(sampleId);
            bool supported = true;
            for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
            {
                supported = supported && faults[sa].size() <=
                    dynamic_spare::kBistOverlapMaxFaultsPerSa;
            }
            if (!supported)
            {
                ++outOfEnvelope;
                outOfEnvelopeCsv << sampleId << ',' << kDateSeed << ','
                                  << kScenarioId;
                for (std::size_t sa = 0; sa < dynamic_spare::kSubarrayCount; ++sa)
                {
                    outOfEnvelopeCsv << ',' << faults[sa].size();
                }
                outOfEnvelopeCsv << ",OUT_OF_T1_TIMING_ENVELOPE\n";
                continue;
            }
            records.push_back(evaluateSample(sampleId, faults, schedule, model));
        }
        require(records.size() == groupCount - outOfEnvelope,
                "Generated/valid T1 DATE count identity failed");
        writeRawCsv(root / "raw" / (stage + "_groups.csv"), records);
        writeSummary(root / "summary" / (stage + "_scenario_summary.csv"),
                     root / "summary" / (stage + "_per_sa_catchup.csv"),
                     root / "summary" / (stage + "_critical_sa_distribution.csv"),
                     root / "summary" / (stage + "_fault_location_correlation.csv"),
                     root / "tables" / (stage + "_date_summary.md"),
                     root / "logs" / (stage + "_metadata.txt"),
                     records, groupCount, outOfEnvelope);
        writeCdfAndHistogram(root / "cdf" / (stage + "_latency_cdf.csv"),
                             root / "cdf" / (stage + "_latency_histogram.csv"),
                             records);
        writeWorstExamples(root / "tables" / (stage + "_worst_10.csv"), records);
        std::cout << "T1_DATE_LATENCY stage=" << stage
                  << " scenario=" << kScenarioId
                  << " generated=" << groupCount
                  << " valid=" << records.size()
                  << " out_of_t1_envelope=" << outOfEnvelope << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "T1_DATE_LATENCY FAIL: " << error.what() << '\n';
        return 1;
    }
}
