#ifndef CANONICAL_EXPERIMENT_HPP
#define CANONICAL_EXPERIMENT_HPP

#include "HierarchicalRecamSimulator.hpp"
#include "SramRecamModel.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dynamic_spare
{

enum class CanonicalConfigurationId
{
    B0CamNoSharing,
    B1CamSharing,
    B2SramNoSharing,
    B3SramSharing
};

const char *toString(CanonicalConfigurationId id) noexcept;

struct CanonicalConfigurationResult
{
    CanonicalConfigurationId id =
        CanonicalConfigurationId::B0CamNoSharing;
    SimulationConfig groupConfig;
    DeviceRepairResult device;
    // Retained only for the canonical area manifest.  CAM configurations do
    // not need an SRAM organization.
    std::optional<sram_recam::SramRecamConfig> sramConfig;
};

struct CanonicalExperimentConfig
{
    // This is the B1/B3 sharing configuration. B0/B2 are derived by disabling
    // sharing without changing faults, Rs/Cs, word width, or global capacity.
    SimulationConfig sharingConfig;
    HierarchicalRecamConfig hierarchicalConfig;
    sram_recam::SramRecamConfig sramConfig;
};

std::vector<CanonicalConfigurationResult> runCanonicalFour(
    const std::vector<RepairGroupInput> &inputs,
    const CanonicalExperimentConfig &config);

class CanonicalExperimentReporter
{
public:
    static void write(
        const std::filesystem::path &outputDirectory,
        const std::vector<RepairGroupInput> &inputs,
        const std::vector<CanonicalConfigurationResult> &results);
    static void writeAreaManifest(
        const std::filesystem::path &outputDirectory,
        const std::vector<CanonicalConfigurationResult> &results);
};

} // namespace dynamic_spare

#endif
