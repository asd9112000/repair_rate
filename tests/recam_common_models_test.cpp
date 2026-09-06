#include "CamRecamModel.hpp"
#include "SramRecamModel.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void verifyCommonGeometryAndHardware()
{
    dynamic_spare::RecamGeometryConfig common;
    common.rows = 512;
    common.columns = 512;
    common.spareRows = 2;
    common.spareColumns = 2;
    common.channels = 16;
    common.dataWordBits = 16;
    const auto geometry = dynamic_spare::deriveRecamGeometry(common);
    require(geometry.k == 4 && geometry.addressEntries == 4 &&
                geometry.hybridEntries == 4 &&
                geometry.addressEntryBits == 25 &&
                geometry.hybridEntryBits == 19 &&
                geometry.totalStorageBits == 264 &&
                geometry.matrixBits == 16 && geometry.solutionUnits == 6,
            "Technology-independent RECAM geometry changed");

    const auto cam = dynamic_spare::deriveCamHardwareMetrics(common);
    require(cam.storageTechnology ==
                dynamic_spare::BiraStorageTechnology::Cam &&
                cam.addressPhysicalBits == 100 &&
                cam.hybridPhysicalBits == 76 &&
                cam.temporaryPhysicalBits == 88 &&
                cam.totalStorageBits == 264 &&
                cam.totalModeledBits == 280,
            "CAM HardwareMetrics golden changed");

    sram_recam::SramRecamConfig sram;
    sram.rows = 512;
    sram.cols = 512;
    sram.spareRows = 2;
    sram.spareCols = 2;
    sram.channels = 16;
    sram.dataWordBits = 16;
    sram.addressParallelism = 2;
    sram.hybridReadParallelism = 2;
    sram.runtimeParallelism = 2;
    const auto sramHardware =
        sram_recam::deriveCommonHardwareMetrics(sram);
    require(sramHardware.storageTechnology ==
                dynamic_spare::BiraStorageTechnology::Sram &&
                sramHardware.geometry.addressEntries ==
                    cam.geometry.addressEntries &&
                sramHardware.geometry.hybridEntries ==
                    cam.geometry.hybridEntries &&
                sramHardware.addressPhysicalBits == 100 &&
                sramHardware.hybridPhysicalBits == 76 &&
                sramHardware.temporaryPhysicalBits == 88 &&
                sramHardware.comparatorBits == 80 &&
                sramHardware.totalStorageBits == 264 &&
                sramHardware.totalModeledBits == 360,
            "SRAM common HardwareMetrics golden changed");
}

void verifyCamLatencyModel()
{
    dynamic_spare::CamBiraWorkload workload;
    workload.faultsDetected = 3;
    workload.storageEntriesRead = 8;
    workload.activeMatrixCells = 16;
    workload.candidatesGenerated = 6;
    workload.candidatesEvaluated = 6;
    const auto latency = dynamic_spare::modelCamBiraLatency(
        {}, workload);
    require(latency.storageTechnology ==
                dynamic_spare::BiraStorageTechnology::Cam &&
                latency.faultCollectionWorkCycles == 6 &&
                latency.repairAnalysisWorkCycles == 0 &&
                latency.totalWorkCycles == 6,
            "CAM BIRA latency golden changed");
}

} // namespace

int main()
{
    try
    {
        verifyCommonGeometryAndHardware();
        verifyCamLatencyModel();
        std::cout << "Common RECAM geometry/hardware/latency tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Common RECAM model test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
