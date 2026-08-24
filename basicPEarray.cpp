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

using namespace std;



int main(int argc, char *argv[])
{
    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int buf_num = 2;
    std::string reportDir = "./reports/basicPEarray/";
    std::filesystem::create_directories(reportDir); // Ensure the directory exists
    cout << "Input Rs: " << Rs << ", Cs: " << Cs << endl;

    // Create output file for repair record
    vector<bool> overallRepairSuccessList;
    const string filename = reportDir + "RepairRecordFile.txt";
    ofstream outFile(filename);
    if (!outFile.is_open())
    {
        cerr << "Error: Could not open file " << filename << " for writing." << endl;
        return 0;
    }
    outFile << "overall Repair Success( 1 for success, 0 for failure )" << endl;
    outFile << "  -PE0, PE1, PE2, PE3 Repair Success (1 for success, 0 for failure)" << endl;

    // Generate all solutions
    SolGenerator solGenerator(Rs, Cs);
    solGenerator.writeAllSolVectorsToFile(reportDir + "AllSolVectors.txt");
    solGenerator.writeAllSolMatrixsToFile(reportDir + "AllSolMatrixs.txt");

    // Load faults from file
    FaultLoader faultLoader(Rs, Cs, buf_num);
    if (!faultLoader.loadFaults("./fault_generator/faults.faults"))
    {
        cout << "load failed, could not open file or too many faults." << endl;
        return 0;
    }
    faultLoader.printFaultLoader();
    faultLoader.writeFaultMap(reportDir + "fault_map.txt");
    int patNum = faultLoader.faultLists.size() / 4; // 4 PEs in total

    /* ==========  PE arrays  ============
                    0 1
                    2 3
       ===================================*/
    vector<int> repairCntPE = {0, 0, 0, 0};
    int repairCntOverall = 0;
    for (int ii_pattern = 0; ii_pattern < patNum; ii_pattern++)
    {

        FaultList &PE0_faultList = faultLoader.faultLists[ii_pattern * 4 + 0];
        FaultList &PE1_faultList = faultLoader.faultLists[ii_pattern * 4 + 1];
        FaultList &PE2_faultList = faultLoader.faultLists[ii_pattern * 4 + 2];
        FaultList &PE3_faultList = faultLoader.faultLists[ii_pattern * 4 + 3];

        RECAM_PE PE0(Rs, Cs, buf_num);
        PE0.loadFaultsToCAMs(PE0_faultList);
        PE0.printPE();
        PE0.genFaultAnalyzeMatrix();
        PE0.genValidSolList(solGenerator.allSolMatrixsType);
        PE0.writeFaultAnalyzeMatrixToFile(reportDir + "PE0_fault_analyze_matrix.txt");

        RECAM_PE PE1(Rs, Cs, buf_num);
        PE1.loadFaultsToCAMs(PE1_faultList);
        PE1.printPE();
        PE1.genFaultAnalyzeMatrix();
        PE1.genValidSolList(solGenerator.allSolMatrixsType);
        PE1.writeFaultAnalyzeMatrixToFile(reportDir + "PE1_fault_analyze_matrix.txt");

        RECAM_PE PE2(Rs, Cs, buf_num);
        PE2.loadFaultsToCAMs(PE2_faultList);
        PE2.printPE();
        PE2.genFaultAnalyzeMatrix();
        PE2.genValidSolList(solGenerator.allSolMatrixsType);
        PE2.writeFaultAnalyzeMatrixToFile(reportDir + "PE2_fault_analyze_matrix.txt");

        RECAM_PE PE3(Rs, Cs, buf_num);
        PE3.loadFaultsToCAMs(PE3_faultList);
        PE3.printPE();
        PE3.genFaultAnalyzeMatrix();
        PE3.genValidSolList(solGenerator.allSolMatrixsType);
        PE3.writeFaultAnalyzeMatrixToFile(reportDir + "PE3_fault_analyze_matrix.txt");

        PE0.printValidSolList();
        PE1.printValidSolList();
        PE2.printValidSolList();
        PE3.printValidSolList();

        vector < bool > RepairSuccessList = {PE0.RepairSuccess, PE1.RepairSuccess, PE2.RepairSuccess, PE3.RepairSuccess};
        bool overallRepairSuccess = all_of(RepairSuccessList.begin(), RepairSuccessList.end(), [](bool success) { return success; });
        if (overallRepairSuccess) {
            repairCntOverall++;
        }

        outFile << "Pattern " << ii_pattern << ": ";
        outFile << endl <<(overallRepairSuccess ? "1" : "0") << endl;
        for ( int i = 0 ; i < 4 ; ++i){
            outFile << (RepairSuccessList[i] ? "1" : "0") << endl;
            if (RepairSuccessList[i]) {
                repairCntPE[i]++;
            }
        }

    }
    outFile.close();

    const string RepairReport = reportDir + "RepairReport.txt";
    ofstream reportFile(RepairReport);
    if (!reportFile.is_open())
    {
        cerr << "Error: Could not open file " << RepairReport << " for writing." << endl;
        return 0;
    }

    reportFile << "The Pe array has been repaired " << repairCntOverall << " times in "<< patNum << " times simulation." << endl;
    reportFile << "  -Repair rate: " << static_cast<double>(repairCntOverall) / patNum << endl;
    reportFile << "  -PE0 Repair Success Count: " << repairCntPE[0] << ", Repair Rate: " << static_cast<double>(repairCntPE[0]) / patNum << endl;
    reportFile << "  -PE1 Repair Success Count: " << repairCntPE[1] << ", Repair Rate: " << static_cast<double>(repairCntPE[1]) / patNum << endl;
    reportFile << "  -PE2 Repair Success Count: " << repairCntPE[2] << ", Repair Rate: " << static_cast<double>(repairCntPE[2]) / patNum << endl;
    reportFile << "  -PE3 Repair Success Count: " << repairCntPE[3] << ", Repair Rate: " << static_cast<double>(repairCntPE[3]) / patNum << endl;
    reportFile.close();

    return 0;
}
