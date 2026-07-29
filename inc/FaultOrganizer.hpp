#ifndef FAULT_ORGANIZER_HPP
#define FAULT_ORGANIZER_HPP

#include <string>

// Converts a flat fault-address file into the grouped format consumed by
// FaultLoader. Pass -1 to infer the SubarrayGroup count from the input.
bool organizeSimplifiedFaults(
    const std::string &inputFilename,
    const std::string &outputFilename,
    int subarrayGroupCount = -1);

#endif
