#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip> // for std::setw


// #include "inc/RECAM_addressCAM.hpp"
#include "inc/RECAM_PE.hpp"
#include "inc/SolGenerator.hpp"

using namespace std;



int main(int argc, char *argv[])
{
    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int buf_num = 2;
    cout << "Input Rs: " << Rs << ", Cs: " << Cs << endl;

    // Create output file for repair record
    vector<bool> overallRepairSuccessList;
    const string filename = "./RepairRecordFile.txt";
    ofstream outFile(filename);
    if (!outFile.is_open())
    {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return 0;
    }

    // Generate all solutions
    SolGenerator solGenerator(Rs, Cs);

    // Load faults from file
    FaultLoader faultLoader(Rs, Cs, buf_num);
    if (!faultLoader.loadFaults("./fault_generator/faults.faults"))
    {
        cout << "load failed, could not open file or too many faults." << endl;
        return 0;
    }
    faultLoader.printFaultLoader();
    faultLoader.writeFaultMap("./fault_map.txt");


    /* ==========  PE arrays  ============
                    0 1
                    2 3
       ===================================*/

    FaultList &PE0_faultList = faultLoader.faultLists[0];
    FaultList &PE1_faultList = faultLoader.faultLists[1];
    FaultList &PE2_faultList = faultLoader.faultLists[2];
    FaultList &PE3_faultList = faultLoader.faultLists[3];

    RECAM_PE PE0(Rs, Cs, buf_num);
    PE0.loadFaultsToCAMs(PE0_faultList);
    PE0.printPE();
    PE0.genFaultAnalyzeMatrix();
    PE0.genValidSolList(solGenerator.allSolMatrixsType);
    PE0.writeFaultAnalyzeMatrixToFile("./PE0_fault_analyze_matrix.txt");

    RECAM_PE PE1(Rs, Cs, buf_num);
    PE1.loadFaultsToCAMs(PE1_faultList);
    PE1.printPE();
    PE1.genFaultAnalyzeMatrix();
    PE1.genValidSolList(solGenerator.allSolMatrixsType);
    PE1.writeFaultAnalyzeMatrixToFile("./PE1_fault_analyze_matrix.txt");

    RECAM_PE PE2(Rs, Cs, buf_num);
    PE2.loadFaultsToCAMs(PE2_faultList);
    PE2.printPE();
    PE2.genFaultAnalyzeMatrix();
    PE2.genValidSolList(solGenerator.allSolMatrixsType);
    PE2.writeFaultAnalyzeMatrixToFile("./PE2_fault_analyze_matrix.txt");

    RECAM_PE PE3(Rs, Cs, buf_num);
    PE3.loadFaultsToCAMs(PE3_faultList);
    PE3.printPE();
    PE3.genFaultAnalyzeMatrix();
    PE3.genValidSolList(solGenerator.allSolMatrixsType);
    PE3.writeFaultAnalyzeMatrixToFile("./PE3_fault_analyze_matrix.txt");


    PE0.printValidSolList();
    PE1.printValidSolList();
    PE2.printValidSolList();
    PE3.printValidSolList();

    vector < bool > RepairSuccessList = {PE0.RepairSuccess, PE1.RepairSuccess, PE2.RepairSuccess, PE3.RepairSuccess};
    bool overallRepairSuccess = all_of(RepairSuccessList.begin(), RepairSuccessList.end(), [](bool success) { return success; });
    outFile << "overall Repair Success( 1 for success, 0 for failure )" << endl;
    outFile << "  -PE0, PE1, PE2, PE3 Repair Success (1 for success, 0 for failure)" << endl;
    outFile << (overallRepairSuccess ? "1" : "0") << endl;
    for ( int i = 0 ; i < 4 ; ++i){
        outFile << (RepairSuccessList[i] ? "1" : "0") << endl;
    }

    outFile.close();
    cout << "Fault analyze matrix has been written to " << filename << endl;

    return 0;
}