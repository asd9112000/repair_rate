#include "Vrecam_dss_v2_rs3cs3m1_policy_equivalence_top.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <random>

namespace {
constexpr unsigned kSeed = 20260910, kSaCount = 4, kSlotCount = 4;
struct Map { bool valid = false; unsigned pattern = 0; };
struct Ledger { std::array<bool, 4> released{}; std::array<int, 4> borrower{{-1,-1,-1,-1}}; };
struct Result { bool repairable=false; unsigned failure=3, commits=0, cfg=0, pat=0, donor=0, borrow=0, release=0, ledger=0; };

// This table is inert H1/H2 data. All ranking, feasibility, donor choice,
// traversal, commit, and failure behavior below is independently modeled C++.
unsigned target_config(unsigned sa, unsigned slot) {
    static constexpr unsigned table[4][4]={{0,1,2,3},{0,4,5,6},{0,4,5,6},{0,1,2,3}};
    return table[sa][slot];
}
unsigned release_resource(unsigned sa) { return sa==0 ? 0 : sa==3 ? 1 : sa==1 ? 2 : 3; }
unsigned pack_ledger(const Ledger& l) {
    unsigned out=0;
    for(unsigned r=0;r<4;++r) { if(l.released[r]) out|=1u<<r; if(l.borrower[r]>=0) {
        unsigned id=unsigned(l.borrower[r]); unsigned code=r<2 ? (id==1 ? 1 : 2) : (id==0 ? 1 : 2);
        out|=code<<(4+2*r);
    }}
    return out;
}
unsigned legal_donor(unsigned sa,const Ledger& l) {
    unsigned primary=0, alternate=0;
    if(sa==0) {primary=2;alternate=3;} else if(sa==3) {primary=3;alternate=2;}
    else if(sa==1) {primary=0;alternate=1;} else {primary=1;alternate=0;}
    if(l.released[primary] && l.borrower[primary]<0) return primary;
    if(l.released[alternate] && l.borrower[alternate]<0) return alternate;
    return 4;
}
Result golden(const std::array<Map,16>& maps, bool group) {
    Ledger ledger; Result result;
    for(unsigned sa=0;sa<4;++sa) {
        std::array<unsigned,4> rank{};
        if(group) rank={{1,0,3,2}};
        else if(sa==1 || sa==2) rank={{0,2,1,3}};
        else rank={{0,1,2,3}};
        bool found=false;
        for(unsigned priority=0;priority<4 && !found;++priority) {
            unsigned slot=rank[priority]; const Map candidate=maps[sa*4+slot];
            bool releases=slot==1 || slot==3, borrows=slot==2 || slot==3;
            unsigned donor=borrows ? legal_donor(sa,ledger) : 0;
            if(!candidate.valid || (borrows && donor==4)) continue;
            found=true; result.commits|=1u<<sa; result.cfg|=target_config(sa,slot)<<(3*sa);
            result.pat|=candidate.pattern<<(6*sa); result.donor|=donor<<(2*sa);
            if(releases) result.release|=1u<<sa; if(borrows) result.borrow|=1u<<sa;
            if(releases) ledger.released[release_resource(sa)]=true;
            if(borrows) ledger.borrower[donor]=int(sa);
        }
        if(!found) { result.failure=sa; result.ledger=pack_ledger(ledger); return result; }
    }
    result.repairable=true; result.ledger=pack_ledger(ledger); return result;
}
void tick(Vrecam_dss_v2_rs3cs3m1_policy_equivalence_top& d) { d.clk_i=0;d.eval();d.clk_i=1;d.eval(); }
void load(Vrecam_dss_v2_rs3cs3m1_policy_equivalence_top& d,const std::array<Map,16>& maps) {
    d.candidate_valid_i=0; for(int i=0;i<3;++i)d.candidate_pattern_flat_i[i]=0;
    for(unsigned i=0;i<16;++i) if(maps[i].valid) { d.candidate_valid_i|=1u<<i; unsigned bit=i*6,word=bit/32,off=bit%32;
        d.candidate_pattern_flat_i[word]|=maps[i].pattern<<off;
        if(off>26)d.candidate_pattern_flat_i[word+1]|=maps[i].pattern>>(32-off);
    }
}
void dump_maps(const std::array<Map,16>& maps) {
    for(unsigned sa=0;sa<4;++sa) { std::cerr<<"  SA"<<sa<<":";
        for(unsigned slot=0;slot<4;++slot) { const auto& m=maps[sa*4+slot]; std::cerr<<" slot"<<slot<<"={valid="<<m.valid<<",pattern="<<m.pattern<<"}"; }
        std::cerr<<'\n';
    }
}
bool run(Vrecam_dss_v2_rs3cs3m1_policy_equivalence_top& d,const std::array<Map,16>& maps,bool group,const char* label,unsigned vector) {
    load(d,maps); d.policy_group_i=group; d.rst_ni=0; d.start_i=0; tick(d); d.rst_ni=1; d.start_i=1; tick(d); d.start_i=0;
    for(unsigned cycle=0;cycle<64 && !d.done_o;++cycle) { load(d,maps); tick(d); }
    Result g=golden(maps,group);
    bool ok=d.done_o && bool(d.group_repairable_o)==g.repairable && d.failure_position_o==g.failure && d.sa_commit_valid_o==g.commits &&
        d.selected_config_flat_o==g.cfg && d.selected_pattern_flat_o==g.pat && d.borrow_flat_o==g.borrow && d.release_flat_o==g.release && d.ledger_o==g.ledger;
    // Inactive donor payload is intentionally don't-care.
    for(unsigned sa=0;sa<4;++sa) if((g.borrow>>sa)&1) ok &= ((d.selected_donor_flat_o>>(2*sa)&3)==((g.donor>>(2*sa))&3));
    if(!ok) { std::cerr<<"MISMATCH seed="<<kSeed<<" label="<<label<<" vector="<<vector<<" policy="<<(group?"GROUP":"EARLY")<<'\n'; dump_maps(maps);
        std::cerr<<"  rtl(rep/fail/commit/cfg/pat/bor/rel/ledger)="<<d.group_repairable_o<<"/"<<unsigned(d.failure_position_o)<<"/"<<unsigned(d.sa_commit_valid_o)<<"/"<<unsigned(d.selected_config_flat_o)<<"/"<<unsigned(d.selected_pattern_flat_o)<<"/"<<unsigned(d.borrow_flat_o)<<"/"<<unsigned(d.release_flat_o)<<"/"<<unsigned(d.ledger_o)<<'\n';
        std::cerr<<"  golden(rep/fail/commit/cfg/pat/bor/rel/ledger)="<<g.repairable<<"/"<<g.failure<<"/"<<g.commits<<"/"<<g.cfg<<"/"<<g.pat<<"/"<<g.borrow<<"/"<<g.release<<"/"<<g.ledger<<'\n';
    }
    return ok;
}
std::array<Map,16> locals() { std::array<Map,16> maps{}; for(unsigned sa=0;sa<4;++sa)maps[sa*4]={true,sa+1}; return maps; }
bool directed(const char* name,std::array<Map,16> maps,bool group) { Vrecam_dss_v2_rs3cs3m1_policy_equivalence_top d; bool ok=run(d,maps,group,name,0); std::cout<<"TARGET_DIRECTED "<<name<<" "<<(ok?"PASS":"FAIL")<<'\n'; return ok; }
unsigned directed_suite() {
    unsigned failures=0;
    failures+=!directed("EARLY_LOCAL",locals(),false);
    auto release=locals();release[0]={};release[1]={true,9}; failures+=!directed("EARLY_RELEASE_ONLY",release,false);
    auto primary=locals();primary[2]={};primary[5]={true,10};primary[12]={};primary[14]={true,11};failures+=!directed("EARLY_PRIMARY_DONOR",primary,false);
    auto alternate=locals();alternate[4]={};alternate[5]={true,12};alternate[12]={};alternate[14]={true,13};failures+=!directed("EARLY_ALTERNATE_DONOR",alternate,false);
    auto both=locals();both[0]={};both[1]={true,14};both[4]={};both[5]={};both[7]={true,15};failures+=!directed("EARLY_RELEASE_AND_BORROW",both,false);
    auto conflict=locals();conflict[0]={};conflict[1]={true,16};conflict[4]={};conflict[6]={true,17};conflict[8]={};conflict[10]={true,18};failures+=!directed("EARLY_RESOURCE_CONFLICT",conflict,false);
    const char* early_fail[] = {"EARLY_FAILURE_A","EARLY_FAILURE_B","EARLY_FAILURE_C","EARLY_FAILURE_D"};
    for(unsigned sa=0;sa<4;++sa){auto m=locals();for(unsigned slot=0;slot<4;++slot)m[sa*4+slot]={};failures+=!directed(early_fail[sa],m,false);}
    auto difference=locals();difference[0]={true,19};difference[1]={true,20};failures+=!directed("GROUP_PRIORITY_DIFFERENCE",difference,true);
    failures+=!directed("GROUP_LOCAL",locals(),true);
    const char* group_fail[] = {"GROUP_FAILURE_A","GROUP_FAILURE_B","GROUP_FAILURE_C","GROUP_FAILURE_D"};
    for(unsigned sa=0;sa<4;++sa){auto m=locals();for(unsigned slot=0;slot<4;++slot)m[sa*4+slot]={};failures+=!directed(group_fail[sa],m,true);}
    return failures;
}
}  // namespace

int main(int argc,char**argv) {
    unsigned vectors=argc>1?unsigned(std::stoul(argv[1])):1000, failures=directed_suite(); std::mt19937 rng(kSeed); unsigned mismatches[2]={0,0},paired[4]={0,0,0,0};
    for(unsigned i=0;i<vectors;++i) { std::array<Map,16> maps{}; for(auto& map:maps){map.valid=(rng()%100)<60;map.pattern=1+(rng()%35);} Result early=golden(maps,false),group=golden(maps,true);paired[(early.repairable?0:2)+(group.repairable?0:1)]++; Vrecam_dss_v2_rs3cs3m1_policy_equivalence_top de,dg; if(!run(de,maps,false,"RANDOM",i))++mismatches[0];if(!run(dg,maps,true,"RANDOM",i))++mismatches[1]; }
    std::cout<<"TARGET_EARLY_RANDOM seed="<<kSeed<<" vectors="<<vectors<<" mismatches="<<mismatches[0]<<'\n';
    std::cout<<"TARGET_GROUP_RANDOM seed="<<kSeed<<" vectors="<<vectors<<" mismatches="<<mismatches[1]<<'\n';
    std::cout<<"PAIRED BOTH_PASS="<<paired[0]<<" EARLY_ONLY="<<paired[1]<<" GROUP_ONLY="<<paired[2]<<" BOTH_FAIL="<<paired[3]<<'\n';
    return (failures||mismatches[0]||mismatches[1])?1:0;
}
