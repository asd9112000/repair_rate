#include "../inc/FaultOrganizer.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <faults_simplified.faults> <faults.faults>"
                  << " [subarray_group_count]\n";
        return 1;
    }

    int subarrayGroupCount = -1;
    if (argc == 4)
    {
        try
        {
            subarrayGroupCount = std::stoi(argv[3]);
        }
        catch (const std::exception &error)
        {
            std::cerr << "Error: Invalid subarray group count: "
                      << error.what() << "\n";
            return 1;
        }
    }

    if (!organizeSimplifiedFaults(argv[1], argv[2], subarrayGroupCount))
    {
        return 1;
    }

    std::cout << "Organized faults written to " << argv[2] << "\n";
    return 0;
}
