#include "SramRecamModel.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

struct ResultRow
{
    std::string name;
    std::uint64_t collection = 0;
    std::uint64_t analysis = 0;
    std::uint64_t total = 0;
    std::uint64_t collectionDone = 0;
    std::uint64_t solutionReady = 0;
    std::uint64_t wait = 0;
    std::size_t maxDepth = 0;
    std::uint64_t collectionAfterBist = 0;
    std::uint64_t afterBist = 0;
    std::uint64_t backpressure = 0;
};

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

sram_recam::SramRecamConfig baseConfig()
{
    sram_recam::SramRecamConfig config;
    config.rows = 512;
    config.cols = 512;
    config.spareRows = 2;
    config.spareCols = 2;
    config.channels = 16;
    config.dataWordBits = 16;
    config.biraSearchPolicy = sram_recam::SearchPolicy::SramChunked;
    config.runtimeSearchPolicy = sram_recam::SearchPolicy::SramChunked;
    config.addressParallelism = 2;
    config.hybridReadParallelism = 2;
    config.runtimeParallelism = 2;
    config.matrixPolicy = sram_recam::MatrixPolicy::SramScanRebuild;
    config.stateUpdateCycles = 1;
    config.matrixFinalizeCycles = 1;
    return config;
}

sram_recam::BiraWorkload workload(
    const sram_recam::SramRecamConfig &config)
{
    sram_recam::BiraWorkload work;
    work.faultsDetected = 3;
    work.addressWrites = 3;
    work.analysisPasses = 1;
    for (std::uint64_t occupied = 0; occupied < work.faultsDetected;
         ++occupied)
    {
        const std::uint64_t parallelism =
            config.biraSearchPolicy ==
                    sram_recam::SearchPolicy::SramSerial
                ? 1
                : config.biraSearchPolicy ==
                          sram_recam::SearchPolicy::SramWide
                    ? config.spareRows + config.spareCols
                    : config.addressParallelism;
        work.addressSearchRoundsByFault.push_back(
            occupied == 0
                ? 0
                : occupied / parallelism +
                      (occupied % parallelism != 0));
        work.addressEntriesCompared += occupied;
    }
    work.addressEntryCount = 3;
    work.pivotFaults = 3;
    for (const std::uint64_t rounds : work.addressSearchRoundsByFault)
    {
        work.pivotSearchRounds += rounds;
    }
    return work;
}

ResultRow aggregateRow(
    const std::string &name,
    const sram_recam::SramRecamConfig &config)
{
    const auto result = sram_recam::modelBiraLatency(
        config, workload(config));
    return {name,
            result.faultCollectionCycles,
            result.repairAnalysisCycles,
            result.modeledCycles};
}

ResultRow fifoRow(
    const std::string &name,
    const sram_recam::SramRecamConfig &config)
{
    const auto result = sram_recam::modelDecoupledFifoTimeline(
        config, workload(config), {1, 2, 3}, 5);
    require(result.bistTimelineModeled,
            name + " did not produce an event-driven timeline");
    return {name,
            result.faultCollectionWorkCycles,
            result.repairAnalysisWorkCycles,
            result.totalWorkCycles,
            result.faultCollectionCompletionCycle,
            result.solutionReadyCycle,
            result.totalFaultQueueWaitCycles,
            result.maximumFaultQueueDepth,
            result.faultCollectionLatencyAfterBistCycles,
            result.latencyAfterBistCycles,
            result.bistBackpressureCycles};
}

std::string csv(const std::vector<ResultRow> &rows)
{
    std::ostringstream output;
    output
        << "case,fault_collection_work_cycles,repair_analysis_work_cycles,"
           "total_work_cycles,fault_collection_completion_cycle,"
           "solution_ready_cycle,total_fault_queue_wait_cycles,"
           "maximum_fault_queue_depth,"
           "fault_collection_latency_after_bist_cycles,"
           "latency_after_bist_cycles,bist_backpressure_cycles\n";
    for (const ResultRow &row : rows)
    {
        output << row.name << ',' << row.collection << ',' << row.analysis
               << ',' << row.total << ',' << row.collectionDone << ','
               << row.solutionReady << ',' << row.wait << ',' << row.maxDepth
               << ',' << row.collectionAfterBist << ',' << row.afterBist
               << ',' << row.backpressure << '\n';
    }
    return output.str();
}

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Unable to read SRAM FIFO golden: " +
                                 path.string());
    }
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

void writeFile(const std::filesystem::path &path, const std::string &content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("Unable to write SRAM FIFO result: " +
                                 path.string());
    }
    output << content;
}

} // namespace

int main(int argc, char *argv[])
{
    try
    {
        require(argc == 3,
                "Usage: sram_fault_collection_fifo_test GOLDEN ACTUAL");
        std::vector<ResultRow> rows;

        sram_recam::SramRecamConfig config = baseConfig();
        rows.push_back(aggregateRow("abstract_aggregate", config));

        config.useAbstractSearchRoundModel = false;
        rows.push_back(aggregateRow("registered_aggregate", config));

        config = baseConfig();
        config.biraSearchPolicy = sram_recam::SearchPolicy::SramSerial;
        config.addressParallelism = 1;
        rows.push_back(fifoRow("fifo_serial", config));

        config = baseConfig();
        rows.push_back(fifoRow("fifo_chunk2", config));

        config.biraSearchPolicy = sram_recam::SearchPolicy::SramWide;
        rows.push_back(fifoRow("fifo_wide", config));

        config = baseConfig();
        config.useAbstractSearchRoundModel = false;
        rows.push_back(fifoRow("fifo_registered_chunk2", config));

        config = baseConfig();
        config.matrixPolicy = sram_recam::MatrixPolicy::ShadowMatrix;
        rows.push_back(fifoRow("fifo_shadow_matrix", config));

        const std::string actual = csv(rows);
        writeFile(argv[2], actual);
        const std::string expected = readFile(argv[1]);
        require(actual == expected,
                "SRAM fault-collection/FIFO result differs from golden; "
                "inspect " + std::string(argv[2]));

        std::cout << "SRAM fault collection FIFO tests passed\n"
                  << "Golden: " << argv[1] << '\n'
                  << "Actual: " << argv[2] << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "SRAM fault collection FIFO test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
