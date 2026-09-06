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
#include "inc/FaultOrganizer.hpp"


using namespace std;
// using PE_SpareLineConfig = vector<std::pair<int, int>>;

struct spareLineConfig
{
    std::pair<int, int> spare [4];
};


class PatternRecorder
{
public:
    struct FailedSubarrayGroup
    {
        int HBMID;
        int ChannelID;
        int BankID;
        int SubarrayGroupID;
    };

    string reportDir = "./reports/legacy/sharedline_sram/";
    string repairReportPath = reportDir + "RepairReport.rpt";
    string repairRecordPath = reportDir + "RepairRecord.txt";
    string remapTablePath = "./reports/legacy/sharedline_sram/RemapTable.txt";
    string simplifiedRemapTablePath = "./reports/legacy/sharedline_sram/RemapTable_simplified.txt";
    ofstream repairRecordFile;
    ofstream repairReportFile;
    ofstream remapTableFile;
    ofstream simplifiedRemapTableFile;
    int nextRemapOptionId = 0;

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
    vector<FailedSubarrayGroup> failedSubarrayGroups;

    explicit PatternRecorder(
        int bufferEntriesPerPE = 0,
        bool paperCamReuse = false)
    {
        std::filesystem::create_directories(reportDir); // Ensure the directory exists
        remapTableFile.open(remapTablePath);
        simplifiedRemapTableFile.open(simplifiedRemapTablePath);
        if (remapTableFile.is_open())
        {
            const bool bufferDisabled = !paperCamReuse && bufferEntriesPerPE == 0;
            remapTableFile << "# REMAP_TABLE_LOG 2\n";
            remapTableFile << "# CAM_REUSE_MODE "
                           << (paperCamReuse
                               ? "PAPER_ADDITIONAL_PIVOT"
                               : (bufferDisabled
                                   ? "BUFFER_DISABLED"
                                   : "FIXED_ADDITIONAL_PIVOT_BUFFER")) << "\n";
            if (paperCamReuse)
            {
                remapTableFile << "# BUFFER_ENTRIES_PER_PE Rs+Cs\n";
            }
            else
            {
                remapTableFile << "# BUFFER_ENTRIES_PER_PE "
                               << bufferEntriesPerPE << "\n";
            }
            remapTableFile << "# HYBRID_OVERFLOW_TO_BUFFER_EXTENSION "
                           << 0 << "\n";
            remapTableFile << "# OPTION <pattern_id> <option_id> <config_index>\n";
            remapTableFile << "# PE <pe_id> <spare_rows> <spare_cols> <solution_index>\n";
            remapTableFile << "# MAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> <SubarrayID> <r> <c> <R|C> <new_address> <latency>\n";
            remapTableFile << "# BUFFMAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> <SubarrayID> <r> <c> <latency>\n";
            remapTableFile << "#FAILED SUBARRAY GROUP <HBMID> <ChannelID> <BankID> <SubarrayGroupID>\n";
        }
        if (!simplifiedRemapTableFile.is_open())
        {
            cerr << "Error: Could not open " << simplifiedRemapTablePath << " for writing." << endl;
        }
    }

    ~PatternRecorder()
    {
        if (remapTableFile.is_open())
        {
            remapTableFile.close();
        }
        if (simplifiedRemapTableFile.is_open())
        {
            simplifiedRemapTableFile.close();
        }
    }

    void writeRemapOption(
        int patternId,
        int configIndex,
        const spareLineConfig &config,
        const std::array<const RECAM_PE *, 4> &pes,
        const std::array<size_t, 4> &tableIndices)
    {
        if (!remapTableFile.is_open())
        {
            cerr << "Error: Could not open " << remapTablePath << " for writing." << endl;
            return;
        }

        const int optionId = nextRemapOptionId++;
        remapTableFile << "OPTION " << patternId << " " << optionId << " " << configIndex << "\n";
        for (int peIndex = 0; peIndex < 4; ++peIndex)
        {
            const RECAM_PE &pe = *pes[peIndex];
            const size_t tableIndex = tableIndices[peIndex];
            remapTableFile << "PE " << peIndex << " "
                           << config.spare[peIndex].first << " "
                           << config.spare[peIndex].second << " "
                           << pe.validSolList[tableIndex] << "\n";
            for (const RemapTable::RemapEntry &entry : pe.remapTableList[tableIndex].RemapEntries)
            {
                const RemapTable::AddressEntry &address = entry.addressEntry;
                auto writeMapEntry = [&](std::ostream &output)
                {
                    output << "MAP "
                           << address.HBMID << " "
                           << address.ChannelID << " "
                           << address.BankID << " "
                           << address.SubarrayGroupID << " "
                           << address.SubarrayID << " "
                           << address.r << " "
                           << address.c << " "
                           << (entry.isSpareRow ? 'R' : 'C') << " "
                           << entry.newRowColAddr << " "
                           << entry.Latency << "\n";
                };
                writeMapEntry(remapTableFile);
                if (simplifiedRemapTableFile.is_open())
                {
                    writeMapEntry(simplifiedRemapTableFile);
                }
            }
            for (const RemapTable::BufferRemapEntry &entry :
                 pe.remapTableList[tableIndex].BufferRemapEntries)
            {
                const RemapTable::AddressEntry &address = entry.addressEntry;
                auto writeBufferMapEntry = [&](std::ostream &output)
                {
                    output << "BUFFMAP "
                           << address.HBMID << " "
                           << address.ChannelID << " "
                           << address.BankID << " "
                           << address.SubarrayGroupID << " "
                           << address.SubarrayID << " "
                           << address.r << " "
                           << address.c << " "
                           << entry.Latency << "\n";
                };
                writeBufferMapEntry(remapTableFile);
                if (simplifiedRemapTableFile.is_open())
                {
                    writeBufferMapEntry(simplifiedRemapTableFile);
                }
            }
            remapTableFile << "END_PE\n";
        }
        remapTableFile << "END_OPTION\n";
    }

    void recordFailedPattern(
        int patternId,
        const std::array<const FaultList *, 4> &faultLists)
    {
        for (int peIndex = 0; peIndex < 4; ++peIndex)
        {
            if (faultLists[peIndex] != nullptr && !faultLists[peIndex]->PEFaults.empty())
            {
                const Fault &fault = faultLists[peIndex]->PEFaults.front();
                failedSubarrayGroups.push_back({
                    fault.HBMID,
                    fault.ChannelID,
                    fault.BankID,
                    fault.SubarrayGroupID});
                return;
            }
        }
        cerr << "Warning: Could not determine the address of failed pattern "
             << patternId << " because all four subarrays are empty." << endl;
    }

    void writeFailedPatterns()
    {
        if ((!remapTableFile.is_open() && !simplifiedRemapTableFile.is_open()) ||
            failedSubarrayGroups.empty())
        {
            return;
        }

        for (const FailedSubarrayGroup &group : failedSubarrayGroups)
        {
            auto writeFailedSubarrayGroup = [&](std::ostream &output)
            {
                output << "#FAILED SUBARRAY GROUP "
                       << group.HBMID << " "
                       << group.ChannelID << " "
                       << group.BankID << " "
                       << group.SubarrayGroupID << "\n";
            };

            if (remapTableFile.is_open())
            {
                writeFailedSubarrayGroup(remapTableFile);
            }
            if (simplifiedRemapTableFile.is_open())
            {
                writeFailedSubarrayGroup(simplifiedRemapTableFile);
            }
        }
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

const RECAM_PE *getPEForSpareConfig(const FourWayPE &fourWayPE, int spareRows, int spareCols)
{
    if (spareRows == fourWayPE.Rs && spareCols == fourWayPE.Cs)
        return &fourWayPE.PE_RsCs;
    if (spareRows == fourWayPE.RsRuduced && spareCols == fourWayPE.Cs)
        return &fourWayPE.PE_RsReduced;
    if (spareRows == fourWayPE.Rs && spareCols == fourWayPE.CsRuduced)
        return &fourWayPE.PE_CsReduced;
    if (spareRows == fourWayPE.RsRuduced && spareCols == fourWayPE.CsRuduced)
        return &fourWayPE.PE_RsCsReduced;
    return nullptr;
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "Error: Missing spare row/column arguments.\n"
             << "Usage: " << argv[0]
             << " <Rs> <Cs> [--paper-cam-reuse | --buffer <entries>]"
             << " [--rptName <name>]\n"
             << "Example: " << argv[0] << " 2 2 --paper-cam-reuse\n";
        return 1;
    }

    int Rs = atoi(argv[1]); // number of spare rows
    int Cs = atoi(argv[2]); // number of spare columns
    int RsRuduced = Rs - 1;
    int CsRuduced = Cs - 1;
    int buf_num = 2;
    int sharedLine_num = 1;
    bool paperCamReuse = false;
    bool fixedBufferSpecified = false;

    for (int i = 3; i < argc; ++i)
    {
        if (argv[i] == string("--buffer"))
        {
            fixedBufferSpecified = true;
            if (i + 1 >= argc)
            {
                cerr << "Error: --buffer requires a nonnegative entry count.\n";
                return 1;
            }
            const string bufferArgument = argv[++i];
            size_t parsedCharacters = 0;
            try
            {
                buf_num = stoi(bufferArgument, &parsedCharacters);
            }
            catch (const std::exception &)
            {
                cerr << "Error: --buffer must be a nonnegative integer.\n";
                return 1;
            }
            if (parsedCharacters != bufferArgument.size() || buf_num < 0)
            {
                cerr << "Error: --buffer must be a nonnegative integer.\n";
                return 1;
            }
        }
        else if (argv[i] == string("--paper-cam-reuse"))
        {
            paperCamReuse = true;
        }
    }
    if (paperCamReuse && fixedBufferSpecified)
    {
        cerr << "Error: --paper-cam-reuse and --buffer are mutually exclusive.\n";
        return 1;
    }

    if (paperCamReuse)
    {
        cout << "Buffer CAM mode: paper additional-pivot reuse "
             << "(capacity = local Rs+Cs)." << endl;
    }
    else if (buf_num == 0)
    {
        cout << "Buffer CAM mode: disabled (capacity = 0). "
             << "CAM overflow makes that PE configuration unrepairable." << endl;
    }
    else
    {
        cout << "Buffer CAM mode: fixed-capacity additional-pivot buffer "
             << "(capacity = " << buf_num << ")." << endl;
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
    PatternRecorder patternRecorder(buf_num, paperCamReuse);
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

    string unorganizedFaultFilePath ;
    string organizedFaultFilePath = "./fault_generator/faults.faults";

    // ===============   To Ming-Jie, Wang ====================
    // Change these two paths when using a different unorganized fault file.
    // unorganizedFaultFilePath = "./fault_generator/faults_simplified.faults";
    // organizedFaultFilePath ="./fault_generator/faults_organized.faults";
    // if (!organizeSimplifiedFaults(
    //         unorganizedFaultFilePath,
    //         organizedFaultFilePath))
    // {
    //     cerr << "Error: Failed to organize fault file "
    //          << unorganizedFaultFilePath << endl;
    //     return 1;
    // }
    // ==========================================================



    // Load the same faults for different PE spare line configurations
    FaultLoaderForSpares faultLoaderForSpares(
        Rs, Cs, buf_num, paperCamReuse);
    for (const auto& config : spareLineConfigs) {
        int r_spare = config.first;
        int c_spare = config.second;
        if (!faultLoaderForSpares.generateFaultListsForSpares(
                r_spare, c_spare, organizedFaultFilePath))
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
                const auto &configs = (PE_index == 0 || PE_index == 3)
                    ? spareLineConfigsLessRow
                    : spareLineConfigsLessCol;
                int r_spare = configs[SL_index].first;
                int c_spare = configs[SL_index].second;
                faultListsForSpares[PE_index][SL_index] = faultLoaderForSpares.faultListsForSpares[{r_spare, c_spare}][ii_pattern *4 + PE_index];
            }
        }


        // =======================================================
        // for PE0 PE3 with max spare lines ( Rs, Cs + sharedLine_num)
        // for PE1 PE2 with max spare lines ( Rs + sharedLine_num, Cs)
        // =======================================================
        std::array<FourWayPE, 4> fourWayPEs = {
            FourWayPE(Rs, Cs + sharedLine_num, buf_num, 0, paperCamReuse),
            FourWayPE(Rs + sharedLine_num, Cs, buf_num, 1, paperCamReuse),
            FourWayPE(Rs + sharedLine_num, Cs, buf_num, 2, paperCamReuse),
            FourWayPE(Rs, Cs + sharedLine_num, buf_num, 3, paperCamReuse)};


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
        bool patternRepairSuccess = false;
        bool firstRemapTableWritten = false;
        cout << "Start checking spare line configurations for pattern " << ii_pattern << endl;
        // Check all combinations of spare line configurations for the four PEs
        for ( const auto &configLessRow0 : spareLineConfigsLessRow) { // PE0
            for (const auto &configLessCol1 : spareLineConfigsLessCol){ // PE1
                for ( const auto & configLessCol2 : spareLineConfigsLessCol){ //PE2
                    for (const auto &configLessRow3 : spareLineConfigsLessRow){ // PE3
                        configIndex++;
                        resSpareLines.resetResSpareLines();
                        bool thisConfigRepairSuccess = true;
                        bool PERepairSuccessList[4] = {false, false, false, false};
                        int occupiedRows[4] = {configLessRow0.first, configLessCol1.first, configLessCol2.first, configLessRow3.first};
                        int occupiedCols[4] = {configLessRow0.second, configLessCol1.second, configLessCol2.second, configLessRow3.second};


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
                            patternRepairSuccess = true;
                            patternRecorder.addPERepairSuccConfigMap((ii_pattern), configIndex, config);

                            std::array<const RECAM_PE *, 4> selectedPEs{};
                            for (int peIndex = 0; peIndex < 4; ++peIndex)
                            {
                                selectedPEs[peIndex] = getPEForSpareConfig(
                                    fourWayPEs[peIndex], occupiedRows[peIndex], occupiedCols[peIndex]);
                            }
                            const bool hasValidRemapTables = std::all_of(
                                selectedPEs.begin(), selectedPEs.end(),
                                [](const RECAM_PE *pe) { return pe != nullptr && !pe->remapTableList.empty(); });
                            if (hasValidRemapTables && !firstRemapTableWritten)
                            {
                                patternRecorder.writeRemapOption(
                                    ii_pattern, configIndex, config, selectedPEs, {0, 0, 0, 0});
                                firstRemapTableWritten = true;
                            }

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

        if (!patternRepairSuccess)
        {
            std::array<const FaultList *, 4> failedFaultLists{};
            for (int peIndex = 0; peIndex < 4; ++peIndex)
            {
                // Index 1 corresponds to the original (Rs, Cs) configuration.
                failedFaultLists[peIndex] = &faultListsForSpares[peIndex][1];
            }
            patternRecorder.recordFailedPattern(ii_pattern, failedFaultLists);
        }
    }

    patternRecorder.writeFailedPatterns();
    patternRecorder.analyzeRecord();
    patternRecorder.writeRepairRecord();
    patternRecorder.writeRepairReport();

    cout << "RepairRate: " << patternRecorder.repairRate  << " ";
    cout << "RepairRate_RECAM: " << patternRecorder.repairRate_RECAM << " ";
    cout << "SpareLine: " << Rs << " ";


    return 0;
}
