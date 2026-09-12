#include "Vdss_analyzer_top.h"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
struct Fault{unsigned r,c;};
struct Selection{unsigned cfg=0,pat=0;std::vector<unsigned> rows,cols;};
struct TestCase{std::string name;bool group=false,success=false;std::array<std::vector<Fault>,4> faults;std::array<std::array<unsigned,7>,4> patterns{};std::array<Selection,4> selected;};
static void require(bool v,const std::string&m){if(!v){std::cerr<<"dss_analyzer_top_golden_test: "<<m<<'\n';std::exit(1);}}
static void tick(Vdss_analyzer_top&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();d.clk_i=0;d.eval();}
static unsigned bits(const WData*words,unsigned offset,unsigned width){unsigned word=offset/32,shift=offset%32;unsigned long long value=words[word];if(shift+width>32)value|=static_cast<unsigned long long>(words[word+1])<<32;return (value>>shift)&((1U<<width)-1U);}
static std::vector<TestCase> load(const char*path){
    std::ifstream in(path);require(static_cast<bool>(in),"cannot open golden vector file");std::vector<TestCase> cases;std::string line;
    while(std::getline(in,line)){if(line.empty())continue;std::istringstream s(line);std::string tag;s>>tag;
        if(tag=="CASE"){cases.emplace_back();s>>cases.back().name>>cases.back().group>>cases.back().success;}
        else if(tag=="SA"){unsigned sa,n;s>>sa>>n;for(unsigned i=0,r,c;i<n;++i){s>>r>>c;cases.back().faults[sa].push_back({r,c});}for(auto&p:cases.back().patterns[sa])s>>p;}
        else if(tag=="SEL"){unsigned sa,n,value;s>>sa>>cases.back().selected[sa].cfg>>cases.back().selected[sa].pat>>n;for(unsigned i=0;i<n;++i){s>>value;cases.back().selected[sa].rows.push_back(value);}s>>n;for(unsigned i=0;i<n;++i){s>>value;cases.back().selected[sa].cols.push_back(value);}}
    }return cases;
}
static void pulse(Vdss_analyzer_top&d,CData&signal){signal=1;tick(d);signal=0;d.eval();}
int main(int argc,char**argv){require(argc==2||argc==3,"golden vector path and optional --area are required");const bool area_mode=argc==3&&std::string(argv[2])=="--area";require(argc==2||area_mode,"unknown test option");auto cases=load(argv[1]);require(!cases.empty(),"golden file contains no cases");Vdss_analyzer_top d;
    d.rst_ni=0;d.clear_group_i=0;d.subarray_start_i=0;d.subarray_commit_i=0;d.fault_valid_i=0;d.selection_start_i=0;d.policy_group_i=0;tick(d);d.rst_ni=1;
    for(const auto&tc:cases){pulse(d,d.clear_group_i);d.policy_group_i=tc.group;
        for(unsigned sa=0;sa<4;++sa){d.subarray_i=sa;pulse(d,d.subarray_start_i);for(const auto&f:tc.faults[sa]){require(d.fault_ready_o,tc.name+": collector not ready");d.fault_row_i=f.r;d.fault_col_i=f.c;d.fault_valid_i=1;tick(d);d.fault_valid_i=0;d.eval();}pulse(d,d.subarray_commit_i);if(area_mode)for(unsigned cycle=0;cycle<42;++cycle)tick(d);
            for(unsigned cfg=0;cfg<7;++cfg)require(bits(d.config_pattern_map_o,(sa*7+cfg)*4,4)==tc.patterns[sa][cfg],tc.name+": ConfigPatternMap mismatch SA"+std::to_string(sa)+" CFG"+std::to_string(cfg));}
        pulse(d,d.selection_start_i);for(unsigned cycle=0;cycle<2505&&!d.selection_done_o;++cycle)tick(d);require(d.selection_done_o,tc.name+": selector timeout");require(static_cast<bool>(d.group_success_o)==tc.success,tc.name+": success differs from C++ golden");
        if(tc.success){for(unsigned sa=0;sa<4;++sa){unsigned actual_cfg=(d.selected_configs_o>>(sa*3))&7U;require(actual_cfg==tc.selected[sa].cfg,tc.name+": selected ConfigID mismatch SA"+std::to_string(sa)+" expected "+std::to_string(tc.selected[sa].cfg)+" actual "+std::to_string(actual_cfg));require(((d.selected_patterns_o>>(sa*4))&15U)==tc.selected[sa].pat,tc.name+": selected PatternID mismatch");}for(unsigned cycle=0;cycle<8&&!d.final_program_valid_o;++cycle)tick(d);require(d.final_program_valid_o,tc.name+": successful group did not complete PendingRepairBuffer decode");
            for(unsigned sa=0;sa<4;++sa){std::vector<unsigned> rows,cols;for(unsigned p=0;p<5;++p){if((d.repair_row_valid_o>>(sa*5+p))&1U)rows.push_back(bits(d.repair_rows_flat_o,(sa*5+p)*10,10));if((d.repair_col_valid_o>>(sa*5+p))&1U)cols.push_back(bits(d.repair_cols_flat_o,(sa*5+p)*10,10));}require(rows==tc.selected[sa].rows,tc.name+": decoded row addresses mismatch");require(cols==tc.selected[sa].cols,tc.name+": decoded column addresses mismatch");}}
        std::cout<<tc.name<<" PASS\n";
    }
    std::cout<<"dss_analyzer_top_golden_test PASS\n";
}
