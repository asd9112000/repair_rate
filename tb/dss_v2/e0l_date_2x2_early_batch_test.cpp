#include "Vrecam_dss_v2_early_core.h"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Row { std::string id; std::array<std::array<unsigned, 7>, 4> pattern{}; };
std::vector<std::string> split(const std::string &s) { std::vector<std::string> v; std::stringstream x(s); std::string p; while (std::getline(x,p,',')) v.push_back(p); return v; }
std::vector<Row> readRows(const char *path) { std::ifstream in(path); std::string line; std::getline(in,line); std::vector<Row> rows; while (std::getline(in,line)) { const auto f=split(line); if(f.size()!=36) throw std::runtime_error("invalid candidate CSV row"); Row r; r.id=f[0]; for(unsigned sa=0;sa<4;++sa) for(unsigned cfg=0;cfg<7;++cfg) r.pattern[sa][cfg]=static_cast<unsigned>(std::stoul(f[8+sa*7+cfg])); rows.push_back(r); } return rows; }
void tick(Vrecam_dss_v2_early_core &d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
void reset(Vrecam_dss_v2_early_core &d){d.clk_i=0;d.rst_ni=0;d.start_i=0;d.candidate_solution_valid_i=0;d.candidate_repairable_i=0;d.candidate_pattern_id_i=0;tick(d);d.rst_ni=1;}
void run(Vrecam_dss_v2_early_core &d,const Row&r,std::ofstream&o){reset(d);d.start_i=1;tick(d);d.start_i=0;std::array<int,4> edge{{-1,-1,-1,-1}};unsigned old=0;int done=-1;for(int c=1;c<=40;++c){const unsigned sa=d.current_sa_o,cfg=d.current_config_id_o;if(sa>=4||cfg>=7)throw std::runtime_error("EARLY core index out of range");const unsigned p=r.pattern[sa][cfg];d.candidate_solution_valid_i=p!=0;d.candidate_repairable_i=p!=0;d.candidate_pattern_id_i=p;d.eval();tick(d);const unsigned changed=d.sa_commit_valid_o&~old;for(unsigned i=0;i<4;++i)if(changed&(1u<<i))edge[i]=c;old=d.sa_commit_valid_o;if(d.done_o){done=c;break;}}if(done<0)throw std::runtime_error("EARLY terminal timeout");o<<r.id<<','<<(d.group_repairable_o?1:0)<<','<<(d.group_repairable_o?-1:static_cast<int>(d.failure_position_o))<<','<<done;for(int v:edge)o<<','<<v;o<<','<<d.selected_config_flat_o<<','<<d.selected_pattern_flat_o<<'\n';}
}
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("usage: corpus output");const auto rows=readRows(argv[1]);if(rows.size()!=10000)throw std::runtime_error("EARLY corpus size mismatch");std::ofstream out(argv[2]);out<<"transaction_id,repairable,failure_position,done_cycle,A_commit_cycle,B_commit_cycle,C_commit_cycle,D_commit_cycle,selected_config_flat,selected_pattern_flat\n";Vrecam_dss_v2_early_core dut;for(const auto&r:rows)run(dut,r,out);return 0;}catch(const std::exception&e){std::cerr<<"E0L EARLY FAIL: "<<e.what()<<'\n';return 1;}}
