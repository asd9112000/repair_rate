#include "Vmulti_config_early_selector.h"
#include <cstdlib>
#include <iostream>
#include <vector>
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
static void tick(Vmulti_config_early_selector&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
static void set_map(Vmulti_config_early_selector&d,const std::vector<unsigned>&entries){
    for(auto&w:d.config_pattern_map_i)w=0;
    for(unsigned i=0;i<entries.size();++i){unsigned bit=i*4;d.config_pattern_map_i[bit/32]|=entries[i]<<(bit%32);}
}
int main(){
    {
    std::vector<unsigned> all(28,1);
    Vmulti_config_early_selector probe;set_map(probe,all);probe.rst_ni=0;probe.start_i=0;tick(probe);probe.rst_ni=1;probe.start_i=1;tick(probe);probe.start_i=0;
    for(unsigned n=0;n<20&&!probe.done_o;++n)tick(probe);
    require(probe.done_o&&probe.success_o,"EARLY rejected all-valid map");
    require((probe.selected_configs_o&7U)==1,"EARLY resource priority did not choose CFG1");
    }
    std::vector<unsigned> map(28,0);map[1]=1;map[4]=1;map[7]=1;map[16]=1;map[21]=1;
    Vmulti_config_early_selector d;set_map(d,map);d.rst_ni=0;d.start_i=0;tick(d);d.rst_ni=1;d.start_i=1;tick(d);d.start_i=0;
    for(unsigned n=0;n<20&&!d.done_o;++n)tick(d);
    require(d.done_o&&!d.success_o,"EARLY unexpectedly backtracked around its greedy loss");
    require((d.selected_configs_o&7U)==1,"EARLY priority did not first choose A=2R1C");
    std::cout<<"early_selector_test PASS\n";
}
