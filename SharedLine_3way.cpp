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

#include "inc/RECAM_PE.hpp"
#include "inc/SolGenerator.hpp"
#include "inc/FourWayPE.hpp"
#include "inc/ResSpareLines.hpp"


using namespace std;
// using PE_SpareLineConfig = vector<std::pair<int, int>>;

struct spareLineConfig
{
    std::pair<int, int> spare [4];
};


class PatternRecorder
{
public:
    string reportDir = "./Report_SharedLine_3way/";
    string repairReportPath = reportDir + "RepairReport.rpt";
    string repairRecordPath = reportDir + "RepairRecord.txt";
    ofstream repairRecordFile;
    ofstream repairReportFile;

    // record details for "every" pattern
    double patNum = 0;
    vector<bool> repairRecord;
    vector<spareLineConfig> repairRecordConfig; // pair<patIndex, configIndex>


    // record for success pattern
    double repairRate = 0.0;
    double repairRate_RECAM = 0.0;
    vector<int> repairSuccessPatternList;
    vector<int> repairSuccessPatternList_RECAM;
    unordered_map<int, vector<spareLineConfig>> PERepairSuccConfigMap;


    PatternRecorder()
    {
        std::filesystem::create_directories(reportDir); // Ensure the directory exists
    }

    void setReportFileName(string rptName)
    {
        repairReportPath = reportDir + rptName;
    }

    void setRecordFileName(string recordName)
    {
        repairRecordPath = reportDir + recordName;
    }

    void addPERepairSuccConfigMap(int patIndex, int configIndex, const spareLineConfig &config)
    {
        if (PERepairSuccConfigMap.find(patIndex) == PERepairSuccConfigMap.end())
        {
            PERepairSuccConfigMap[patIndex] = vector<spareLineConfig>();
        }
        PERepairSuccConfigMap[patIndex].push_back(config);
    }


    void writeRepairRecord()
    {
        repairRecordFile.open(repairRecordPath);
        if (!repairRecordFile.is_open())
        {
            cerr << "Error: Could not open file " << repairRecordPath << " for writing." << endl;
            return;
        }

        for (auto entry : repairRecord)
        {
            repairRecordFile << entry << endl;
        }
    }

    void analyzeRecord()
    {
        repairRate = repairSuccessPatternList.size() / patNum;
        repairRate_RECAM = repairSuccessPatternList_RECAM.size() / patNum;
    }

    void writeRepairReport()
    {
        repairReportFile.open(repairReportPath);
        if (!repairReportFile.is_open())
        {
            cerr << "Error: Could not open file " << repairReportPath << " for writing." << endl;
            return;
        }

        repairReportFile << endl;
        repairReportFile << " =========== Repair Success Patterns: " << repairSuccessPatternList.size() <<" / "<< patNum <<" ( " << repairRate * 100 << "% )"<<" ===========" << endl;
        // for (auto entry : repairSuccessPatternList)
        for (int SuccCnt = 0; SuccCnt < repairSuccessPatternList.size(); ++SuccCnt)
        {
            repairReportFile << std::setw(4) << std::right << repairSuccessPatternList[SuccCnt] << " ";
            if ((SuccCnt % 32) == 31)
            {
                repairReportFile << endl;
            }
        }
        repairReportFile << endl << endl;

        repairReportFile << " =========== RECAM Cconfig Success Patterns: " << repairSuccessPatternList_RECAM.size() << " / " << patNum << " ( " << repairRate_RECAM * 100 << "% )" << " ===========" << endl;
        for (int SuccCnt = 0; SuccCnt < repairSuccessPatternList_RECAM.size(); ++SuccCnt)
        {
            // 設定數字寬度為6位數，右對齊
            repairReportFile << std::setw(4) << std::right << repairSuccessPatternList_RECAM[SuccCnt] << " ";
            if ((SuccCnt % 32) == 31)
            {
                repairReportFile << endl;
            }
        }
        repairReportFile << endl
                         << endl;

        for ( int i = 0 ; i < repairSuccessPatternList.size(); ++i){
            int patIndex = repairSuccessPatternList[i];
            repairReportFile << "Pattern " << patIndex << " successful repair configs: " << endl;
            for (const auto &config : PERepairSuccConfigMap[patIndex]){
                repairReportFile << "  - Config: ";
                for (int peIndex = 0; peIndex < 4; ++peIndex){
                    repairReportFile << "(PE" << peIndex << ": " << config.spare[peIndex].first << "R, " << config.spare[peIndex].second << "C) ";
                }
                repairReportFile << endl;
            }

        }
    }

};


int main(int argc, char *argv[])
{

    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int RsRuduced = Rs - 1;
    int CsRuduced = Cs - 1;
    int buf_num = 2;
    int sharedLine_num = 1;

    int faultNum = 0;
    for(int i = 0; i < argc; ++i){
        if (argv[i] == string("--faultNum") && i + 1 < argc){
            faultNum = atoi(argv[i+1]);
        }
    }

    /*
        +---+---+
        | A | B |
        +---+---+
        | C | D |
        +---+---+

    Block A, B, C, D are arranged in a 2x2 grid, each with dimensions Rs x Cs.

    - **Physical View**:
        - Block A, D:
            - Cs spare columns
            - Rs spare rows (sharedLine_num : Rs - sharedLine_num)
        - Block B, C:
            - Rs spare rows
            - Cs spare columns (sharedLine_num : Cs - sharedLine_num)

    - **Functional View**:
        - Block A, D: max (Rs, Cs + sharedLine_num)
            - Spare columns: Cs ~ (Cs + sharedLine_num)
            - Spare rows: (Rs - sharedLine_num) ~ Rs
        - Block B, C: max (Rs + sharedLine_num, Cs)
            - Spare rows: Rs ~ (Rs + sharedLine_num)
            - Spare columns: (Cs - sharedLine_num) ~ Cs

    */

    vector<pair<int, int>> spareLineConfigs = {
        // RS, Cs
        // RsResuced
        // CsReduced
        // RsCsReduced

        // max (Rs, Cs + sharedLine_num)
        {Rs, Cs + sharedLine_num},
        {Rs - sharedLine_num, Cs + sharedLine_num},
        {Rs, Cs},
        {Rs - sharedLine_num, Cs},

        //  max (Rs + sharedLine_num, Cs)
        {Rs + sharedLine_num, Cs},
        // {Rs, Cs},
        {Rs + sharedLine_num, Cs - sharedLine_num},
        {Rs, Cs - sharedLine_num}

    };

    vector<pair<int, int>> spareLineConfigsLessRow = {
        // max (Rs, Cs + sharedLine_num)
        {Rs, Cs + sharedLine_num},
        {Rs - sharedLine_num, Cs + sharedLine_num},
        {Rs, Cs},
        {Rs - sharedLine_num, Cs}
    };

    vector<pair<int, int>> spareLineConfigsLessCol = {
        //  max (Rs + sharedLine_num, Cs)
        {Rs + sharedLine_num, Cs},
        {Rs, Cs},
        {Rs + sharedLine_num, Cs - sharedLine_num},
        {Rs, Cs - sharedLine_num}
    };

    // ===================================
    //  Record files
    // ===================================
    PatternRecorder patternRecorder;
    for ( int i = 0; i < argc; ++i){
        if ( argv[i] == string("--rptName") && i + 1 < argc){
            patternRecorder.setReportFileName(argv[i+1]);
        }
    }

    // ===================================
    // Generate all solutions
    // ===================================
    SolGenerator solGenerator(Rs, Cs);
    for (const auto& config : spareLineConfigs) {
        int r_spare = config.first;
        int c_spare = config.second;
        solGenerator.genSolForSpares(r_spare, c_spare);
    }
    // solGenerator.writeAllSolVecForSparesToFile(patternRecorder.reportDir + "AllSolVecForSpares");
    // solGenerator.writeAllSolMatForSparesToFile(patternRecorder.reportDir + "AllSolMatForSpares");


    // ===================================
    // Load faults from file
    // ===================================
    // Load the same faults for different PE spare line configurations
    FaultLoaderForSpares faultLoaderForSpares(Rs, Cs, buf_num);
    for (const auto& config : spareLineConfigs) {
        int r_spare = config.first;
        int c_spare = config.second;
        if (!faultLoaderForSpares.generateFaultListsForSpares(r_spare, c_spare, "./fault_generator/faults.faults"))
        {
            cout << "load failed, could not open file or too many faults." << endl;
            return 0;
        }
    }


    // ===================================
    //  Load faults into PE arrays
    //              0 1
    //              2 3
    // ===================================
    int patNum = faultLoaderForSpares.faultListsForSpares[{Rs, Cs}].size() / 4; // number of fault patterns, using the (Rs, Cs) configuration as reference
    patternRecorder.patNum = patNum;
    cout << "Total fault patterns: " << patNum << endl;
    // statistics
    int redundantLineCnt = 0;
    int allWayRepairCount = 0;
    int PERepairCount = 0;
    int maxFaultCnt = 0;
    int minFaultCnt = 10000;
    vector<int> perPERepairCount = {0, 0, 0, 0};

    // [PE indeex][ spare line config index ] -> list of fault lists for different patterns
    vector<vector<FaultList>> faultListsForSpares(4, vector<FaultList>(4, FaultList(Rs, Cs, buf_num)));

    for (int ii_pattern = 0; ii_pattern < patNum; ii_pattern ++)
    {
        // =======================================================
        // Load faults for pattern ii_pattern
        //     - into 4 PEs
        //     - with different spare line configurations
        // =======================================================
        for (int PE_index = 0; PE_index < 4; ++PE_index)
        { // PE index
            for (int SL_index = 0; SL_index < 4; ++SL_index)
            { // spare line config index
                int r_spare = spareLineConfigsLessRow[SL_index].first;
                int c_spare = spareLineConfigsLessRow[SL_index].second;
                faultListsForSpares[PE_index][SL_index] = faultLoaderForSpares.faultListsForSpares[{r_spare, c_spare}][ii_pattern *4 + PE_index];
            }
        }


        // =======================================================
        // for PE0 PE3 with max spare lines ( Rs, Cs + sharedLine_num)
        // for PE1 PE2 with max spare lines ( Rs + sharedLine_num, Cs)
        // =======================================================
        std::array<FourWayPE, 4> fourWayPEs = {
            FourWayPE(Rs, Cs + sharedLine_num, buf_num, 0),
            FourWayPE(Rs + sharedLine_num, Cs, buf_num, 1),
            FourWayPE(Rs + sharedLine_num, Cs, buf_num, 2),
            FourWayPE(Rs, Cs + sharedLine_num, buf_num, 3)};


        // =======================================================
        // for PE0 PE3 with max spare lines ( Rs, Cs + sharedLine_num)
        // =======================================================
        fourWayPEs[0].loadFaultsToPEs(faultListsForSpares[0][0], faultListsForSpares[0][1], faultListsForSpares[0][2], faultListsForSpares[0][3]);
        fourWayPEs[0].genFaultAnalyzeMatrix();
        fourWayPEs[0].genValidSolList(  solGenerator.solMatForSpares[{Rs, Cs + sharedLine_num}],
                                        solGenerator.solMatForSpares[{Rs - sharedLine_num, Cs + sharedLine_num}],
                                        solGenerator.solMatForSpares[{Rs, Cs}],
                                        solGenerator.solMatForSpares[{Rs - sharedLine_num, Cs}]);
        // fourWayPEs[0].writeFaultAnalyzeMatrixToFile(patternRecorder.reportDir + "FourWayPE0_fault_analyze_matrix");
        // fourWayPEs[0].printValidSolList();

        fourWayPEs[3].loadFaultsToPEs(faultListsForSpares[3][0], faultListsForSpares[3][1], faultListsForSpares[3][2], faultListsForSpares[3][3]);
        fourWayPEs[3].genFaultAnalyzeMatrix();
        fourWayPEs[3].genValidSolList(  solGenerator.solMatForSpares[{Rs, Cs + sharedLine_num}],
                                        solGenerator.solMatForSpares[{Rs - sharedLine_num, Cs + sharedLine_num}],
                                        solGenerator.solMatForSpares[{Rs, Cs}],
                                        solGenerator.solMatForSpares[{Rs - sharedLine_num, Cs}]);
        // fourWayPEs[3].writeFaultAnalyzeMatrixToFile(patternRecorder.reportDir + "FourWayPE3_fault_analyze_matrix");
        // fourWayPEs[3].printValidSolList();


        // =======================================================
        // for PE1 PE2 with max spare lines ( Rs + sharedLine_num, Cs)
        // =======================================================
        fourWayPEs[1].loadFaultsToPEs(faultListsForSpares[1][0], faultListsForSpares[1][1], faultListsForSpares[1][2], faultListsForSpares[1][3]);
        fourWayPEs[1].genFaultAnalyzeMatrix();
        fourWayPEs[1].genValidSolList(  solGenerator.solMatForSpares[{Rs + sharedLine_num, Cs}],
                                        solGenerator.solMatForSpares[{Rs, Cs}],
                                        solGenerator.solMatForSpares[{Rs + sharedLine_num, Cs - sharedLine_num}],
                                        solGenerator.solMatForSpares[{Rs, Cs - sharedLine_num}]);
        // fourWayPEs[1].writeFaultAnalyzeMatrixToFile(patternRecorder.reportDir + "FourWayPE1_fault_analyze_matrix");
        // fourWayPEs[1].printValidSolList();

        fourWayPEs[2].loadFaultsToPEs(faultListsForSpares[2][0], faultListsForSpares[2][1], faultListsForSpares[2][2], faultListsForSpares[2][3]);
        fourWayPEs[2].genFaultAnalyzeMatrix();
        fourWayPEs[2].genValidSolList(  solGenerator.solMatForSpares[{Rs + sharedLine_num, Cs}],
                                        solGenerator.solMatForSpares[{Rs, Cs}],
                                        solGenerator.solMatForSpares[{Rs + sharedLine_num, Cs - sharedLine_num}],
                                        solGenerator.solMatForSpares[{Rs, Cs - sharedLine_num}]);

        // fourWayPEs[2].writeFaultAnalyzeMatrixToFile(patternRecorder.reportDir + "FourWayPE2_fault_analyze_matrix");
        // fourWayPEs[2].printValidSolList();

        // Priority of repair success:
        // PE0 -> PE1 -> PE2 -> PE3
        // 1. PE0, PE3: Spare row might be borrow
        //     - RsLessCs > RsCs > RsLessCsPlus > RsCsPlus
        // 2. PE1, PE2: Spare col might be borrow
        //     - CsLessRs > RsCs > CsLessRsPlus > RsCsPlus
        ResSpareLines resSpareLines(Rs, Cs, sharedLine_num);

        int configIndex = -1;
        cout << "Start checking spare line configurations for pattern " << ii_pattern << endl;
        for ( const auto &configLessRow0 : spareLineConfigsLessRow) { // PE0
            for (const auto &configLessCol1 : spareLineConfigsLessCol){ // PE1
                for ( const auto & configLessRow2 : spareLineConfigsLessRow){ //PE2
                    for ( const auto & configLessCol3 : spareLineConfigsLessCol){ // PE3
                        configIndex++;
                        resSpareLines.resetResSpareLines();
                        bool thisConfigRepairSuccess = true;
                        bool PERepairSuccessList[4] = {false, false, false, false};
                        int occupiedRows[4] = {configLessRow0.first, configLessCol1.first, configLessRow2.first, configLessCol3.first};
                        int occupiedCols[4] = {configLessRow0.second, configLessCol1.second, configLessRow2.second, configLessCol3.second};


                        for (int i = 0; i < 4; ++i){
                            if ( (fourWayPEs[i].perWayRepairSuccessListForSparess[{occupiedRows[i], occupiedCols[i]}]))
                            {
                                PERepairSuccessList[i] = resSpareLines.occupySpareLines(i, occupiedRows[i], occupiedCols[i]);
                                if (!PERepairSuccessList[i])
                                {
                                    thisConfigRepairSuccess = false;
                                    break;
                                }
                            }
                            else {
                                //TODO : record
                                thisConfigRepairSuccess = false;
                                break;
                            }
                        }

                        spareLineConfig config = {
                            {{occupiedRows[0], occupiedCols[0]},
                            {occupiedRows[1], occupiedCols[1]},
                            {occupiedRows[2], occupiedCols[2]},
                            {occupiedRows[3], occupiedCols[3]}}};
                        patternRecorder.repairRecordConfig.push_back(config);
                        patternRecorder.repairRecord.push_back(thisConfigRepairSuccess);


                        if (thisConfigRepairSuccess)
                        {
                            patternRecorder.addPERepairSuccConfigMap((ii_pattern), configIndex, config);

                            // Check if this pattern is a new successful repair pattern
                            bool isNewPattern = patternRecorder.repairSuccessPatternList.empty() ||
                                                patternRecorder.repairSuccessPatternList.back()  != ii_pattern;
                            if (isNewPattern){
                                patternRecorder.repairSuccessPatternList.push_back(ii_pattern);
                            }

                            // Check if this pattern is also successfully repaired by the RECAM configuration ( Rs, Cs )
                            // Need not check whether the config is a new one for this pattern.
                            bool isRECAMConfig = true;
                            for (int i_c = 0; i_c < 4; i_c++){
                                isRECAMConfig = isRECAMConfig && (occupiedRows[i_c] == Rs) && (occupiedCols[i_c] == Cs);
                            }
                            if (isRECAMConfig){
                                patternRecorder.repairSuccessPatternList_RECAM.push_back(ii_pattern);
                            }
                        }
                    }
                }
            }
        }
    }

    patternRecorder.analyzeRecord();
    patternRecorder.writeRepairRecord();
    patternRecorder.writeRepairReport();

    // cout << "RepairRate: " << patternRecorder.repairRate  << " ";
    // cout << "RepairRate_RECAM: " << patternRecorder.repairRate_RECAM << " ";
    // cout << "SpareLine: " << Rs << " ";
    // cout << "faultNum: " << faultNum << endl;


    return 0;
}
