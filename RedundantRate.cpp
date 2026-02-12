/*
    This programs aims to know the repair efficiency of different spare line configurations (Rs, Cs) for a PE array.
      - The same fault patterns
      - Different spare line configurations: (Rs, Cs), (Rs-1, Cs), (Rs, Cs-1), (Rs-1, Cs-1)
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip>    // for std::setw
#include <filesystem> // for std::filesystem::create_directories

// #include "inc/RECAM_addressCAM.hpp"
#include "inc/RECAM_PE.hpp"
#include "inc/SolGenerator.hpp"

using namespace std;

int main(int argc, char *argv[])
{

    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int RsRuduced = Rs - 1;
    int CsRuduced = Cs - 1;
    int buf_num = 2;
    std::string reportDir = "./RedundantRate/";
    string RepairReport = reportDir + "RepairReport.rpt";
    // const string RepairReport = reportDir + "RepairReport" + " " + to_string(Rs) + " " + to_string(Cs) + ".rpt";
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("--rptName"))
        {
            RepairReport = reportDir + "RepairReport" + std::string(argv[++i]) + ".rpt";
        }
    }

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
    SolGenerator solGenerator_RsCs(Rs, Cs);
    SolGenerator solGenerator_RsReduced(RsRuduced, Cs);
    SolGenerator solGenerator_CsReduced(Rs, CsRuduced);
    SolGenerator solGenerator_RsCsReduced(RsRuduced, CsRuduced);

    solGenerator_RsCs.writeAllSolVectorsToFile(reportDir + "AllSolVectorsRsCs.txt");
    solGenerator_RsCs.writeAllSolMatrixsToFile(reportDir + "AllSolMatrixsRsCs.txt");
    solGenerator_RsReduced.writeAllSolVectorsToFile(reportDir + "AllSolVectorsRsReduced.txt");
    solGenerator_RsReduced.writeAllSolMatrixsToFile(reportDir + "AllSolMatrixsRsReduced.txt");
    solGenerator_CsReduced.writeAllSolVectorsToFile(reportDir + "AllSolVectorsCsReduced.txt");
    solGenerator_CsReduced.writeAllSolMatrixsToFile(reportDir + "AllSolMatrixsCsReduced.txt");
    solGenerator_RsCsReduced.writeAllSolVectorsToFile(reportDir + "AllSolVectorsRsCsReduced.txt");
    solGenerator_RsCsReduced.writeAllSolMatrixsToFile(reportDir + "AllSolMatrixsRsCsReduced.txt");


    // Load faults from file
    FaultLoader faultLoader_RsCs(Rs, Cs, buf_num);
    FaultLoader faultLoader_RsReduced(RsRuduced, Cs, buf_num);
    FaultLoader faultLoader_CsReduced(Rs, CsRuduced, buf_num);
    FaultLoader faultLoader_RsCsReduced(RsRuduced, CsRuduced, buf_num);

    if (!faultLoader_RsCs.loadFaults("./fault_generator/faults.faults"))
    {
        cout << "load failed, could not open file or too many faults." << endl;
        return 0;
    }
    if (!faultLoader_RsReduced.loadFaults("./fault_generator/faults.faults"))
    {
        cout << "load failed, could not open file or too many faults." << endl;
        return 0;
    }
    if (!faultLoader_CsReduced.loadFaults("./fault_generator/faults.faults"))
    {
        cout << "load failed, could not open file or too many faults." << endl;
        return 0;
    }
    if (!faultLoader_RsCsReduced.loadFaults("./fault_generator/faults.faults"))
    {
        cout << "load failed, could not open file or too many faults." << endl;
        return 0;
    }

    // faultLoader_RsCs.printFaultLoader();
    faultLoader_RsCs.writeFaultMap(reportDir + "fault_map_RSCS.txt");
    faultLoader_RsReduced.writeFaultMap(reportDir + "fault_map_RsReduced.txt");
    faultLoader_CsReduced.writeFaultMap(reportDir + "fault_map_CsReduced.txt");
    faultLoader_RsCsReduced.writeFaultMap(reportDir + "fault_map_RsCsReduced.txt");

    int patNum = faultLoader_RsCs.faultLists.size() / 4; // 4 PEs in total

    /* ==========  PE arrays  ============
                    0 1
                    2 3
       ===================================*/
    vector<int> repairCntPE = {0, 0, 0, 0};
    int repairCntOverall = 0;
    int redundantLineCnt = 0;
    for (int ii_pattern = 0; ii_pattern < patNum; ii_pattern++)
    {

        FaultList &PE_RsCs_faultList = faultLoader_RsCs.faultLists[ii_pattern ];
        FaultList &PE_RsReduced_faultList = faultLoader_RsReduced.faultLists[ii_pattern ];
        FaultList &PE_CsReduced_faultList = faultLoader_CsReduced.faultLists[ii_pattern ];
        FaultList &PE_RsCsReduced_faultList = faultLoader_RsCsReduced.faultLists[ii_pattern ];

        RECAM_PE PE_RsCs(Rs, Cs, buf_num);
        PE_RsCs.loadFaultsToCAMs(PE_RsCs_faultList);
        PE_RsCs.printPE();
        PE_RsCs.genFaultAnalyzeMatrix();
        PE_RsCs.genValidSolList(solGenerator_RsCs.allSolMatrixsType);
        PE_RsCs.writeFaultAnalyzeMatrixToFile(reportDir + "PE_RsCs_fault_analyze_matrix.txt");

        RECAM_PE PE_RsReduced(RsRuduced, Cs, buf_num);
        PE_RsReduced.loadFaultsToCAMs(PE_RsReduced_faultList);
        PE_RsReduced.printPE();
        PE_RsReduced.genFaultAnalyzeMatrix();
        PE_RsReduced.genValidSolList(solGenerator_RsReduced.allSolMatrixsType);
        PE_RsReduced.writeFaultAnalyzeMatrixToFile(reportDir + "PE_RsReduced_fault_analyze_matrix.txt");

        RECAM_PE PE_CsReduced(Rs, CsRuduced, buf_num);
        PE_CsReduced.loadFaultsToCAMs(PE_CsReduced_faultList);
        PE_CsReduced.printPE();
        PE_CsReduced.genFaultAnalyzeMatrix();
        PE_CsReduced.genValidSolList(solGenerator_CsReduced.allSolMatrixsType);
        PE_CsReduced.writeFaultAnalyzeMatrixToFile(reportDir + "PE_CsReduced_fault_analyze_matrix.txt");

        RECAM_PE PE_RsCsReduced(RsRuduced, CsRuduced, buf_num);
        PE_RsCsReduced.loadFaultsToCAMs(PE_RsCsReduced_faultList);
        PE_RsCsReduced.printPE();
        PE_RsCsReduced.genFaultAnalyzeMatrix();
        PE_RsCsReduced.genValidSolList(solGenerator_RsCsReduced.allSolMatrixsType);
        PE_RsCsReduced.writeFaultAnalyzeMatrixToFile(reportDir + "PE_RsCsReduced_fault_analyze_matrix.txt");

        PE_RsCs.printValidSolList();
        PE_RsReduced.printValidSolList();
        PE_CsReduced.printValidSolList();
        PE_RsCsReduced.printValidSolList();

        vector<bool> RepairSuccessList = {PE_RsCs.RepairSuccess, PE_RsReduced.RepairSuccess, PE_CsReduced.RepairSuccess, PE_RsCsReduced.RepairSuccess};
        bool overallRepairSuccess = all_of(RepairSuccessList.begin(), RepairSuccessList.end(), [](bool success)
                                           { return success; });
        if (overallRepairSuccess)
        {
            repairCntOverall++;
        }


        if (overallRepairSuccess ){
            if (PE_RsCsReduced.RepairSuccess){
                redundantLineCnt++;
                redundantLineCnt++;
            } else if (PE_RsReduced.RepairSuccess || PE_CsReduced.RepairSuccess){
                redundantLineCnt++;
            }
        }
        else if (PE_RsReduced.RepairSuccess || PE_CsReduced.RepairSuccess || PE_RsCsReduced.RepairSuccess){
            if (PE_RsCsReduced.RepairSuccess){
                redundantLineCnt++;
            }
        }

        outFile << "Pattern " << ii_pattern << ": ";
        outFile << endl << "overallRepairSuccess: "
                << (overallRepairSuccess ? "1" : "0") << endl;
        for (int i = 0; i < 4; ++i)
        {
            if ( i == 0){
                outFile << "  -PE_RsCs        Repair Success: " << (RepairSuccessList[i] ? "1" : "0") << endl;
                outFile << "  -validSolution: ";
                for (int solIndex : PE_RsCs.validSolList) {
                    outFile << solIndex << " ";
                }
                outFile << endl;
            } else if (i == 1){
                outFile << "  -PE_RsReduced   Repair Success: " << (RepairSuccessList[i] ? "1" : "0") << endl;
                outFile << "  -validSolution: ";
                for (int solIndex : PE_RsReduced.validSolList) {
                    outFile << solIndex << " ";
                }
                outFile << endl;
            } else if (i == 2){
                outFile << "  -PE_CsReduced   Repair Success: " << (RepairSuccessList[i] ? "1" : "0") << endl;
                outFile << "  -validSolution: ";
                for (int solIndex : PE_CsReduced.validSolList) {
                    outFile << solIndex << " ";
                }
                outFile << endl;
            } else if (i == 3){
                outFile << "  -PE_RsCsReduced Repair Success: " << (RepairSuccessList[i] ? "1" : "0") << endl;
                outFile << "  -validSolution: ";
                for (int solIndex : PE_RsCsReduced.validSolList) {
                    outFile << solIndex << " ";
                }
                outFile << endl;
            }
            // outFile << (RepairSuccessList[i] ? "1" : "0") << endl;
            if (RepairSuccessList[i])
            {
                repairCntPE[i]++;
            }
        }
    }
    outFile.close();

    // const string RepairReport = reportDir + "RepairReport.rpt";
    ofstream reportFile(RepairReport);
    if (!reportFile.is_open())
    {
        cerr << "Error: Could not open file " << RepairReport << " for writing." << endl;
        return 0;
    }

    reportFile << "The Pe array has been repaired " << repairCntOverall << " times in " << patNum << " times simulation." << endl;
    reportFile << "  -Repair rate: " << static_cast<double>(repairCntOverall) / patNum << endl;
    reportFile << "  -PE_RsCs        Repair Success Count: " << repairCntPE[0] << ", Repair Rate: " << static_cast<double>(repairCntPE[0]) / patNum << endl;
    reportFile << "  -PE_RsReduced   Repair Success Count: " << repairCntPE[1] << ", Repair Rate: " << static_cast<double>(repairCntPE[1]) / patNum << endl;
    reportFile << "  -PE_CsReduced   Repair Success Count: " << repairCntPE[2] << ", Repair Rate: " << static_cast<double>(repairCntPE[2]) / patNum << endl;
    reportFile << "  -PE_RsCsReduced Repair Success Count: " << repairCntPE[3] << ", Repair Rate: " << static_cast<double>(repairCntPE[3]) / patNum << endl;

    reportFile << "  -Redundant Line Count: " << redundantLineCnt << ", Average: " << static_cast<double>(redundantLineCnt) / patNum << endl;

    reportFile.close();

    return 0;
}