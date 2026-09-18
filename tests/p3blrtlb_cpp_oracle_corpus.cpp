#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "RepairAttemptSolver.hpp"
#include "SimulationConfig.hpp"
#include "SolGenerator.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>

namespace {
constexpr std::size_t kCases = 1000;
constexpr std::uint32_t kSeed = 20260920;
constexpr std::size_t kSlots = 180;

struct Candidate { bool valid = false; std::size_t rows = 0; std::size_t cols = 0; std::size_t pattern = 0; };
using Case = std::array<Candidate, kSlots>;
std::size_t slot(std::size_t sa, std::size_t attempt, std::size_t pattern) { return sa * 45 + attempt * 15 + pattern; }
std::size_t choose(std::size_t n, std::size_t r) { r = std::min(r, n-r); std::size_t v=1; for(std::size_t i=1;i<=r;++i)v=v*(n-r+i)/i; return v; }

class CorpusSolver final : public dynamic_spare::RepairAttemptSolver {
public:
    explicit CorpusSolver(const std::array<Case,kCases> &corpus) : corpus_(corpus) {}
    dynamic_spare::RepairAttemptResult solve(const std::vector<Fault>&, const dynamic_spare::RECAMSolverRequest &request) const override {
        using namespace dynamic_spare;
        RepairAttemptResult out;
        out.runIndex=request.runIndex; out.attemptIndex=request.attemptIndex; out.stage=request.stage; out.subarrayId=request.subarrayId;
        out.availableRows=request.availableRows; out.availableColumns=request.availableColumns;
        out.provisionedRows=request.provisionedRows; out.provisionedColumns=request.provisionedColumns;
        const std::size_t sa=static_cast<std::size_t>(request.subarrayId), attempt=request.attemptIndex;
        const std::size_t dimension=static_cast<std::size_t>(request.availableRows+request.availableColumns);
        out.candidateSolutions=choose(dimension, static_cast<std::size_t>(request.availableRows));
        out.candidateSolutionsEvaluated=out.candidateSolutions;
        TileSolutionState state; state.subarrayId=request.subarrayId; state.spareRows=request.availableRows; state.spareColumns=request.availableColumns;
        state.matrixRowAddresses.resize(dimension); state.matrixColumnAddresses.resize(dimension);
        state.validSolutionBitmap.assign(out.candidateSolutions,false); state.compressedStorageBits=1;
        const Case &record=corpus_.at(request.runIndex);
        for(std::size_t pattern=0;pattern<out.candidateSolutions && pattern<15;++pattern) {
            const Candidate &entry=record[slot(sa,attempt,pattern)];
            if(!entry.valid) continue;
            state.validSolutionBitmap[pattern]=true;
            SolGenerator generator(request.availableRows,request.availableColumns);
            const solVector &orientation=generator.allSolVectorsType[pattern];
            std::size_t rows=0,cols=0;
            for(std::size_t i=0;i<orientation.size();++i) {
                if(orientation[i] && cols++ < entry.cols) state.matrixColumnAddresses[i]=dynamic_spare::MatrixRepairAddress{0,0,0,0,request.subarrayId,int(i),int(i)};
                if(!orientation[i] && rows++ < entry.rows) state.matrixRowAddresses[i]=dynamic_spare::MatrixRepairAddress{0,0,0,0,request.subarrayId,int(i),int(i)};
            }
        }
        for(std::size_t id=0;id<state.validSolutionBitmap.size();++id) if(state.validSolutionBitmap[id]) {
            const DecodedSolution decoded=decodeSolution(state,id);
            CandidateRepairOption option; option.candidateIndex=id; option.usedRows=decoded.sourceRows.size(); option.usedColumns=decoded.sourceColumns.size();
            out.validCandidateIndices.push_back(id); out.validCandidateOptions.push_back(option);
        }
        out.repairSuccess=!out.validCandidateOptions.empty(); out.isRepairable=out.repairSuccess; out.failedCandidates=out.candidateSolutions-out.validCandidateOptions.size();
        out.tileSolutionState=std::move(state); return out;
    }
private: const std::array<Case,kCases> &corpus_;
};

std::array<Case,kCases> makeCorpus() {
    std::array<Case,kCases> corpus{}; std::mt19937 rng(kSeed);
    for(Case &record:corpus) for(std::size_t sa=0;sa<4;++sa) {
        const std::size_t attempts=(sa==0||sa==3)?2:3;
        for(std::size_t attempt=0;attempt<attempts;++attempt) {
            const std::size_t maxRows=2+attempt, count=choose(maxRows+2,maxRows);
            if((rng()%100)<72) {
                const std::size_t pattern=rng()%count;
                Candidate &entry=record[slot(sa,attempt,pattern)]; entry.valid=true; entry.pattern=pattern;
                entry.rows=rng()%(maxRows+1); entry.cols=rng()%3;
            }
        }
    }
    return corpus;
}

void writeCase(std::ofstream &out,const Case &record,const dynamic_spare::GroupRepairResult &group,const dynamic_spare::PhysicalResourceLedger &ledger) {
    const std::size_t committed=group.groupRepairSuccess ? 4 : group.firstFailureSubarray.value_or(0);
    std::array<dynamic_spare::SpareDemand,4> demands{};
    for(std::size_t sa=0;sa<committed;++sa) demands[sa]={group.selectedCandidateOptions[sa]->usedRows,group.selectedCandidateOptions[sa]->usedColumns};
    const auto allocation=ledger.allocateSequential(demands,committed);
    out << (group.groupRepairSuccess?1:0) << ' ' << group.firstFailureSubarray.value_or(3) << ' ' << allocation.transfers.size();
    for(std::size_t sa=0;sa<4;++sa) out << ' ' << (sa<committed?*group.selectedAttemptIndices[sa]:3) << ' ' << (sa<committed?*group.selectedCandidateIndices[sa]+1:0)
        << ' ' << (sa<committed?group.selectedCandidateOptions[sa]->usedRows:0) << ' ' << (sa<committed?group.selectedCandidateOptions[sa]->usedColumns:0);
    std::array<unsigned,4> donors{{15,15,15,15}}; std::array<unsigned,4> donorCount{};
    for(const auto &transfer:allocation.transfers) if(transfer.dimension==dynamic_spare::SpareDimension::Row) {
        const std::size_t b=transfer.borrowerSubarray; donors[b]&=~(3U<<(donorCount[b]*2)); donors[b]|=(unsigned(transfer.donorSubarray)&3U)<<(donorCount[b]*2); ++donorCount[b];
    }
    for(unsigned donor:donors) out << ' ' << donor;
    for(const auto &line:allocation.lines) if(line.dimension==dynamic_spare::SpareDimension::Row) out << ' ' << (line.assignedSubarray?int(*line.assignedSubarray):4);
    for(const Candidate &entry:record) out << ' ' << entry.valid << ' ' << entry.rows << ' ' << entry.cols;
    out << '\n';
}
}

int main() {
    const auto corpus=makeCorpus();
    dynamic_spare::SimulationConfig config; config.layout=dynamic_spare::GroupLayout::Line1x4; config.topology=dynamic_spare::SharingTopology::NeighborSharing;
    config.sharedRows=1; config.sharedColumns=0; config.modifiers.maximumGroupBorrowedSpares=3; config.solutionTakePolicy=dynamic_spare::SolutionTakePolicy::OneByFourSingleHopEarlyV1;
    const dynamic_spare::PhysicalResourceLedger ledger(config); dynamic_spare::DynamicRepairSimulator simulator(std::make_shared<CorpusSolver>(corpus));
    std::ofstream out("tmp/p3blrtlb_cpp_rtl_oracle.txt"); if(!out) throw std::runtime_error("cannot write oracle corpus"); out << kCases << ' ' << kSeed << '\n';
    for(std::size_t id=0;id<kCases;++id) writeCase(out,corpus[id],simulator.run({},config,id,true),ledger);
    std::cout << "CPP_RTL_SHARED_CORPUS=YES\nCPP_ORACLE_RANDOM_CASES=" << kCases << "\nCPP_ORACLE_SEED=" << kSeed << "\n";
}
