#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw
#include <filesystem> // for std::filesystem::create_directories


// #include "inc/RECAM_addressCAM.hpp"
#include "inc/RECAM_PE.hpp"
#include "inc/SolGenerator.hpp"
#include "inc/fourWayPE.hpp"

using namespace std;



int main(int argc, char *argv[])
{
    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int buf_num = 2;
    std::string reportDir = "./mainReport/";
    std::filesystem::create_directories(reportDir); // Ensure the directory exists
    cout << "Input Rs: " << Rs << ", Cs: " << Cs << endl;

    return 0;
}