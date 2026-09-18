#ifndef DYNAMIC_SPARE_SHARING_DYNAMIC_CSV_REPORTER_HPP
#define DYNAMIC_SPARE_SHARING_DYNAMIC_CSV_REPORTER_HPP

#include "RepairResult.hpp"
#include "SimulationConfig.hpp"
#include "DynamicRepairSimulator.hpp"

#include <filesystem>
#include <vector>

namespace dynamic_spare
{

struct SimulationBatch
{
    SimulationConfig config;
    std::vector<GroupRepairResult> runs;
};

class DynamicCsvReporter
{
public:
    static constexpr const char *kPairedCorpusSchemaVersion =
        "dss_paired_corpus_v1";

    static std::string pairedCorpusId(
        const SimulationConfig &config,
        const std::vector<FaultGroup> &groups);
    static void writePairedCorpus(
        const std::filesystem::path &path,
        const SimulationConfig &config,
        const std::vector<FaultGroup> &groups,
        bool append = false);
    static void writePairedPolicyResults(
        const std::filesystem::path &path,
        const SimulationConfig &config,
        const std::vector<FaultGroup> &groups,
        const std::vector<GroupRepairResult> &runs,
        bool append = false);
    static void writeAttempts(
        const std::filesystem::path &path,
        const std::vector<SimulationBatch> &batches,
        bool append = false);
    static void writeRuns(
        const std::filesystem::path &path,
        const std::vector<SimulationBatch> &batches,
        bool append = false);
    static void writeSummary(
        const std::filesystem::path &path,
        const std::vector<SimulationBatch> &batches,
        bool append = false);
};

} // namespace dynamic_spare

#endif
