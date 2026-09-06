#include "SramRecamModel.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

using sram_recam::AddressMatchResult;
using sram_recam::AddressSramEntry;
using sram_recam::AddressSramSearchEngine;
using sram_recam::BiraWorkload;
using sram_recam::RuntimeData;
using sram_recam::RuntimeKey;
using sram_recam::SearchPolicy;
using sram_recam::SramRecamConfig;
using sram_recam::SramRuntimeRepairTable;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

SramRecamConfig baseConfig(std::uint32_t spareRows, std::uint32_t spareCols)
{
    SramRecamConfig config;
    config.rows = 512;
    config.cols = 512;
    config.spareRows = spareRows;
    config.spareCols = spareCols;
    config.channels = 16;
    config.dataWordBits = 16;
    config.biraSearchPolicy = SearchPolicy::SramChunked;
    config.runtimeSearchPolicy = SearchPolicy::SramChunked;
    config.addressParallelism = 2;
    config.hybridReadParallelism = 2;
    config.runtimeParallelism = 2;
    return config;
}

void verifyBitCountGolden()
{
    const auto two = sram_recam::deriveGeometry(baseConfig(2, 2));
    require(two.addressEntries == 4 && two.hybridEntries == 4,
            "Rs=Cs=2 entry counts changed");
    require(two.addressEntryBits == 25 &&
                two.hybridEntryBits == 19,
            "Rs=Cs=2 SRAM entry widths changed");
    require(two.addressLogicalBits == 100 &&
                two.hybridLogicalBits == 76 &&
                two.coreLogicalBits == 176 &&
                two.tempBufferBits == 88 &&
                two.totalStorageBits == 264,
            "Rs=Cs=2 SRAM bit-count golden changed");

    const auto three = sram_recam::deriveGeometry(baseConfig(3, 3));
    require(three.addressEntries == 6 && three.hybridEntries == 12,
            "Rs=Cs=3 entry counts changed");
    require(three.coreLogicalBits == 390 &&
                three.tempBufferBits == 132 &&
                three.totalStorageBits == 522,
            "Rs=Cs=3 SRAM bit-count golden changed");

    const auto four = sram_recam::deriveGeometry(baseConfig(4, 4));
    require(four.addressEntries == 8 && four.hybridEntries == 24,
            "Rs=Cs=4 entry counts changed");
    require(four.coreLogicalBits == 696 &&
                four.tempBufferBits == 176 &&
                four.totalStorageBits == 872,
            "Rs=Cs=4 SRAM bit-count golden changed");
}

void verifyAddressPacking()
{
    SramRecamConfig config = baseConfig(2, 3);
    config.addressParallelism = 2;
    const auto geometry = sram_recam::deriveGeometry(config);
    const auto &address = geometry.addressSram;

    require(geometry.addressEntries == 5,
            "Rs=2,Cs=3 did not produce five Address entries");
    require(address.logicalEntries == 5 && address.parallelEntries == 2,
            "Address SRAM packing metadata is inconsistent");
    require(address.physicalDepth == 3,
            "NA=5,P_A=2 Address SRAM depth must be three");
    require(address.physicalWidth == 2 * geometry.addressEntryBits,
            "NA=5,P_A=2 Address SRAM width is wrong");
    require(address.logicalBits == 5 * geometry.addressEntryBits &&
                address.physicalBits == 6 * geometry.addressEntryBits &&
                address.paddedBits == geometry.addressEntryBits,
            "NA=5,P_A=2 padding was not accounted for");
    require(std::abs(address.utilization - 5.0 / 6.0) < 1.0e-12,
            "NA=5,P_A=2 utilization is wrong");
}

AddressSramEntry addressEntry(std::uint32_t row, std::uint32_t col)
{
    AddressSramEntry entry;
    entry.valid = true;
    entry.row = row;
    entry.col = col;
    return entry;
}

void requireBothMatches(
    const AddressMatchResult &result,
    std::uint64_t expectedRounds,
    const std::string &policy)
{
    require(result.rowMatch && result.rowMatchIndex == 0,
            policy + " search lost the row match at entry 0");
    require(result.colMatch && result.colMatchIndex == 3,
            policy + " search lost the column match at entry 3");
    require(result.chunksRead == expectedRounds,
            policy + " search round count is wrong");
    require(result.entriesCompared == 4,
            policy + " search did not compare every valid entry");
}

void verifyAddressSearch()
{
    AddressSramSearchEngine empty(4);
    const auto emptyResult = empty.search(
        7, 11, SearchPolicy::SramChunked, 2);
    require(emptyResult.chunksRead == 0 &&
                emptyResult.entriesCompared == 0 &&
                emptyResult.occupiedEntries == 0,
            "An empty Address SRAM must bypass search in zero cycles");

    AddressSramSearchEngine engine(4);
    require(engine.insert(addressEntry(7, 40)) &&
                engine.insert(addressEntry(20, 50)) &&
                engine.insert(addressEntry(30, 60)) &&
                engine.insert(addressEntry(70, 11)),
            "Unable to populate the Address SRAM search fixture");

    requireBothMatches(
        engine.search(7, 11, SearchPolicy::SramSerial, 1),
        4,
        "Serial");
    requireBothMatches(
        engine.search(7, 11, SearchPolicy::SramChunked, 2),
        2,
        "Chunked P=2");
    requireBothMatches(
        engine.search(7, 11, SearchPolicy::SramWide, 1),
        1,
        "Wide");

    AddressSramSearchEngine occupiedPrefix(8);
    require(occupiedPrefix.insert(addressEntry(7, 40)) &&
                occupiedPrefix.insert(addressEntry(20, 11)) &&
                occupiedPrefix.insert(addressEntry(30, 60)),
            "Unable to populate the occupied-prefix fixture");
    const auto dualMatch = occupiedPrefix.search(
        7, 11, SearchPolicy::SramChunked, 2);
    require(dualMatch.rowMatchIndex == 0 &&
                dualMatch.colMatchIndex == 1 &&
                dualMatch.chunksRead == 1 &&
                dualMatch.earlyTerminated &&
                dualMatch.occupiedEntries == 3,
            "Strict occupied scan did not stop after both matches");
    const auto singleMatch = occupiedPrefix.search(
        7, 99, SearchPolicy::SramChunked, 2);
    require(singleMatch.rowMatch && !singleMatch.colMatch &&
                singleMatch.chunksRead == 2 &&
                !singleMatch.earlyTerminated &&
                singleMatch.entriesCompared == 3,
            "Strict occupied scan stopped after only one relevant match");
}

void verifyMatrixScanRounds()
{
    SramRecamConfig config = baseConfig(4, 4);
    config.matrixPolicy = sram_recam::MatrixPolicy::SramScanRebuild;
    config.addressParallelism = 4;
    config.hybridReadParallelism = 4;

    BiraWorkload workload;
    workload.analysisPasses = 1;
    const auto latency = sram_recam::modelBiraLatency(config, workload);

    require(latency.matrixAddressReadRounds == 2,
            "Rs=Cs=4,P_A=4 Address matrix scan must take two rounds");
    require(latency.matrixHybridReadRounds == 6,
            "Rs=Cs=4,P_H=4 Hybrid matrix scan must take six rounds");
    require(latency.matrixAddressReadRounds +
                    latency.matrixHybridReadRounds ==
                8,
            "Rs=Cs=4 matrix scan total must be eight rounds");
}

void verifyRuntimeRepairTable()
{
    const RuntimeKey first{0, 10, 20};
    const RuntimeKey second{1, 30, 40};
    const RuntimeKey third{2, 50, 60};
    const RuntimeData normal{false, false, false, false};
    const RuntimeData firstData{true, false, true, false};
    const RuntimeData secondData{false, true, false, true};
    const RuntimeData updatedFirst{true, true, false, false};

    SramRuntimeRepairTable table(
        2, 4, SearchPolicy::SramChunked, 1);

    const auto initialMiss = table.read(first, normal);
    require(!initialMiss.hit && initialMiss.matchIndex == -1 &&
                initialMiss.data == normal,
            "Runtime miss did not return normal DRAM data");

    require(table.insert(first, firstData),
            "Unable to insert the first runtime repair entry");
    const auto firstRead = table.read(first, normal);
    require(firstRead.hit && firstRead.matchIndex == 0 &&
                firstRead.data == firstData,
            "Runtime repair read did not return the first repair word");

    require(table.insert(second, secondData),
            "Unable to insert the second runtime repair entry");
    require(table.size() == 2 && table.capacity() == 2,
            "Runtime repair table occupancy is wrong at full capacity");
    require(!table.insert(third, normal),
            "Full runtime repair table accepted an additional entry");

    const auto secondRead = table.read(second, normal);
    require(secondRead.hit && secondRead.matchIndex == 1 &&
                secondRead.data == secondData,
            "Independent second runtime repair word was lost");

    const auto writeResult = table.write(first, updatedFirst);
    require(writeResult.hit && writeResult.data == updatedFirst,
            "Runtime repair write did not update the matching entry");
    require(table.read(first, normal).data == updatedFirst,
            "Updated runtime repair data was not retained");
    require(table.read(second, normal).data == secondData,
            "Writing one runtime repair entry changed another entry");

    const auto missingWrite = table.write(third, normal);
    require(!missingWrite.hit && table.size() == 2,
            "Runtime miss write unexpectedly allocated an entry");
}

void verifyRuntimeExposedLatency()
{
    SramRecamConfig hidden = baseConfig(2, 2);
    hidden.runtimeSearchPolicy = SearchPolicy::SramSerial;
    hidden.runtimeParallelism = 1;
    hidden.runtimeDataReadCycles = 1;
    hidden.runtimeMuxCycles = 1;
    hidden.normalDramReadyCycles = 3;

    const auto hiddenLatency = sram_recam::modelRuntimeLatency(
        hidden, 1, 1, 1);
    require(hiddenLatency.exposedExtraCycles == 0 &&
                hiddenLatency.exposedExtraCyclesAverage == 0.0,
            "Lookup completed before DRAM ready but exposed extra latency");

    SramRecamConfig exposed = hidden;
    exposed.normalDramReadyCycles = 1;
    const auto exposedLatency = sram_recam::modelRuntimeLatency(
        exposed, 1, 1, 1);
    require(exposedLatency.exposedExtraCycles == 2 &&
                exposedLatency.exposedExtraCyclesAverage == 2.0,
            "Runtime lookup overrun should expose exactly two cycles");

    const auto emptyTableMisses = sram_recam::modelRuntimeLatency(
        exposed, 3, 0, 0);
    require(emptyTableMisses.hits == 0 &&
                emptyTableMisses.misses == 3 &&
                emptyTableMisses.lookupRounds == 12,
            "An empty runtime table must model all requests as misses");
}

void verifyZeroParallelismRejected()
{
    SramRecamConfig invalid = baseConfig(2, 2);
    invalid.addressParallelism = 0;
    bool rejected = false;
    try
    {
        invalid.validate();
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "P_A=0 was not rejected");
}

} // namespace

int main()
{
    try
    {
        verifyBitCountGolden();
        verifyAddressPacking();
        verifyAddressSearch();
        verifyMatrixScanRounds();
        verifyRuntimeRepairTable();
        verifyRuntimeExposedLatency();
        verifyZeroParallelismRejected();
        std::cout << "SRAM_RECAM model tests passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
