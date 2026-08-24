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
#include "inc/FourWayPE.hpp"
// #include "inc/RDR_PEs.hpp"

using namespace std;

int main(int argc, char *argv[])
{

    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int RsRuduced = Rs - 1;
    int CsRuduced = Cs - 1;
    int buf_num = 2;

    // default parameters
    string reportDir = "./reports/RedundantRate/";
    std::filesystem::create_directories(reportDir); // Ensure the directory exists
    // string RepairReport = reportDir + "RepairReport.rpt";


    // ===================================
    //  repair record files
    // ===================================
    string   repairRecordFileName = reportDir + "RepairRecordFile.txt";
    string   reportFileName       = reportDir + "RepairReport.txt";
    ofstream repairRecordFile(repairRecordFileName);

    // set parameters from command line arguments
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("--rptName"))
        {
            reportFileName = reportDir + "RepairReport" + std::string(argv[++i]) + ".rpt";
        }
    }

    if (!repairRecordFile.is_open())
    {
        cerr << "Error: Could not open file " << repairRecordFileName << " for writing." << endl;
        return 0;
    }
    repairRecordFile << "overall Repair Success( 1 for success, 0 for failure )" << endl;
    repairRecordFile << "  -PE0, PE1, PE2, PE3 Repair Success (1 for success, 0 for failure)" << endl;



    // ===================================
    // Generate all solutions
    // ===================================
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


    // ===================================
    // Load faults from file
    // ===================================
    // Load the same faults for different PE spare line configurations
    FaultLoader faultLoader_RsCs(Rs, Cs, buf_num);
    FaultLoader faultLoader_RsReduced(RsRuduced, Cs, buf_num);
    FaultLoader faultLoader_CsReduced(Rs, CsRuduced, buf_num);
    FaultLoader faultLoader_RsCsReduced(RsRuduced, CsRuduced, buf_num);

    std::vector<FaultLoader*> faultLoaders = {  &faultLoader_RsCs,
                                                &faultLoader_RsReduced,
                                                &faultLoader_CsReduced,
                                                &faultLoader_RsCsReduced};
    for (auto* loader : faultLoaders) {
        if (!loader->loadFaults("./fault_generator/faults.faults")) {
            cout << "load failed, could not open file or too many faults." << endl;
            return 0;
        }
    }

    // faultLoader_RsCs.printFaultLoader();
    faultLoader_RsCs.writeFaultMap(reportDir + "fault_map_RSCS.txt");
    faultLoader_RsReduced.writeFaultMap(reportDir + "fault_map_RsReduced.txt");
    faultLoader_CsReduced.writeFaultMap(reportDir + "fault_map_CsReduced.txt");
    faultLoader_RsCsReduced.writeFaultMap(reportDir + "fault_map_RsCsReduced.txt");


    // ===================================
    //  Load faults into PE arrays
    //              0 1
    //              2 3
    // ===================================
    int patNum = faultLoader_RsCs.faultLists.size();

    // statistics
    int redundantLineCnt = 0;
    int allWayRepairCount= 0;
    int PERepairCount    = 0;
    int maxFaultCnt      = 0;
    int minFaultCnt      = 10000;
    vector<int> perPERepairCount = {0, 0, 0, 0};
    // vector<bool> overallRepairSuccessList;


    for (int ii_pattern = 0; ii_pattern < patNum; ii_pattern++)
    {
        FaultList &faultList_RsCs = faultLoader_RsCs.faultLists[ii_pattern ];
        FaultList &faultList_RsReduced = faultLoader_RsReduced.faultLists[ii_pattern ];
        FaultList &faultList_CsReduced = faultLoader_CsReduced.faultLists[ii_pattern ];
        FaultList &faultList_RsCsReduced = faultLoader_RsCsReduced.faultLists[ii_pattern ];

        if (faultList_RsCs.PEFaults.size() > maxFaultCnt)
        {
            maxFaultCnt = faultList_RsCs.PEFaults.size();
        }
        if (faultList_RsCs.PEFaults.size() < minFaultCnt){
            minFaultCnt = faultList_RsCs.PEFaults.size();
        }

        FourWayPE rdrPEs(Rs, Cs, buf_num, 0);
        rdrPEs.loadFaultsToPEs(faultList_RsCs, faultList_RsReduced, faultList_CsReduced, faultList_RsCsReduced);
        rdrPEs.genFaultAnalyzeMatrix();
        rdrPEs.genValidSolList(solGenerator_RsCs.allSolMatrixsType,
                                  solGenerator_RsReduced.allSolMatrixsType,
                                  solGenerator_CsReduced.allSolMatrixsType,
                                  solGenerator_RsCsReduced.allSolMatrixsType);
        rdrPEs.writeFaultAnalyzeMatrixToFile(reportDir + "FourWayPE_fault_analyze_matrix");
        rdrPEs.printValidSolList();


        if (rdrPEs.allWayRepairSuccess)
        {
            allWayRepairCount++;
        }

        // detecting special cases( I think these cases should not happen )
        if (rdrPEs.PE_RsCsReduced.RepairSuccess){
            if ( ! (rdrPEs.PE_RsReduced.RepairSuccess && rdrPEs.PE_CsReduced.RepairSuccess && rdrPEs.PE_RsCs.RepairSuccess) ){
                cout << "Error: special case detected, RsCsReduced repair success but some of others don't  success." << endl;
            }
        }
        else if (rdrPEs.PE_RsReduced.RepairSuccess || rdrPEs.PE_CsReduced.RepairSuccess){
            if ( ! rdrPEs.PE_RsCs.RepairSuccess){
                cout << "Error: special case detected, RsReduced or CsReduced repair success but RsCs doesn't success." << endl;
            }
        }

        if (rdrPEs.PE_RsCsReduced.RepairSuccess)
        {
            if (rdrPEs.PE_RsCs.RepairSuccess)
            {
                redundantLineCnt = redundantLineCnt + 2;
            }
            else if (rdrPEs.PE_RsReduced.RepairSuccess || rdrPEs.PE_CsReduced.RepairSuccess  )
            {
                redundantLineCnt++;
            }
        }
        else if (rdrPEs.PE_RsReduced.RepairSuccess || rdrPEs.PE_CsReduced.RepairSuccess)
        {
            if (rdrPEs.PE_RsCs.RepairSuccess)
            {
                redundantLineCnt++;
            }
        }


        repairRecordFile << "Pattern " << ii_pattern << ": ";
        repairRecordFile << endl
                         << "rdrPEs.allWayRepairSuccess: "
                         << (rdrPEs.allWayRepairSuccess ? "1" : "0") << endl;
        for (int i = 0; i < 4; ++i)
        {
            if (i == 0)
            {
                repairRecordFile << "  -PE_RsCs Repair Success: " << (rdrPEs.perWayRepairSuccessList[i] ? "1" : "0") << endl;
                repairRecordFile << "  -validSolution: ";
                for (int solIndex : rdrPEs.PE_RsCs.validSolList)
                {
                    repairRecordFile << solIndex << " ";
                }
                repairRecordFile << endl;
            }
            else if (i == 1)
            {
                repairRecordFile << "  -PE_RsReduced   Repair Success: " << (rdrPEs.perWayRepairSuccessList[i] ? "1" : "0") << endl;
                repairRecordFile << "  -validSolution: ";
                for (int solIndex : rdrPEs.PE_RsReduced.validSolList)
                {
                    repairRecordFile << solIndex << " ";
                }
                repairRecordFile << endl;
            }
            else if (i == 2)
            {
                repairRecordFile << "  -PE_CsReduced   Repair Success: " << (rdrPEs.perWayRepairSuccessList[i] ? "1" : "0") << endl;
                repairRecordFile << "  -validSolution: ";
                for (int solIndex : rdrPEs.PE_CsReduced.validSolList)
                {
                    repairRecordFile << solIndex << " ";
                }
                repairRecordFile << endl;
            }
            else if (i == 3)
            {
                repairRecordFile << "  -PE_RsCsReduced Repair Success: " << (rdrPEs.perWayRepairSuccessList[i] ? "1" : "0") << endl;
                repairRecordFile << "  -validSolution: ";
                for (int solIndex : rdrPEs.PE_RsCsReduced.validSolList)
                {
                    repairRecordFile << solIndex << " ";
                }
                repairRecordFile << endl;
            }
            // repairRecordFile << (RepairSuccessList[i] ? "1" : "0") << endl;
            if (rdrPEs.perWayRepairSuccessList[i])
            {
                perPERepairCount[i]++;
            }
        }
    }
    repairRecordFile.close();

    ofstream reportFile(reportFileName);
    if (!reportFile.is_open())
    {
        cerr << "Error: Could not open file " << reportFileName << " for writing." << endl;
        return 0;
    }
    // reportFile << "The PE array has been repaired " << PERepairCount << " times in " << patNum << " times simulation." << endl;
    reportFile << "  -Max fault count of all patterns: " << maxFaultCnt << endl;
    reportFile << "  -Min fault count of all patterns: " << minFaultCnt << endl;
    reportFile << "  -Repair rate: " << static_cast<double>(PERepairCount) / patNum << endl;
    reportFile << "  -PE_RsCs        Repair Success Count: " << perPERepairCount[0] << ", Repair Rate: " << static_cast<double>(perPERepairCount[0]) / patNum << endl;
    reportFile << "  -PE_RsReduced   Repair Success Count: " << perPERepairCount[1] << ", Repair Rate: " << static_cast<double>(perPERepairCount[1]) / patNum << endl;
    reportFile << "  -PE_CsReduced   Repair Success Count: " << perPERepairCount[2] << ", Repair Rate: " << static_cast<double>(perPERepairCount[2]) / patNum << endl;
    reportFile << "  -PE_RsCsReduced Repair Success Count: " << perPERepairCount[3] << ", Repair Rate: " << static_cast<double>(perPERepairCount[3]) / patNum << endl;

    reportFile << "  -Redundant Line Count: " << redundantLineCnt << ", Average: " << static_cast<double>(redundantLineCnt) / patNum << endl;

    reportFile.close();

    return 0;
}

