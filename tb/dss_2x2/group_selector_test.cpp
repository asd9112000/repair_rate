#include "Vmulti_config_group_selector.h"
#include <cstdlib>
#include <iostream>
#include <vector>
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
static void tick(Vmulti_config_group_selector&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
static void set_map(Vmulti_config_group_selector&d,const std::vector<unsigned>&entries){
    for(auto&w:d.config_pattern_map_i)w=0;
    for(unsigned i=0;i<entries.size();++i){unsigned bit=i*4;d.config_pattern_map_i[bit/32]|=entries[i]<<(bit%32);}
}
static void run(Vmulti_config_group_selector&d){d.start_i=1;tick(d);d.start_i=0;for(unsigned n=0;n<2500&&!d.done_o;++n)tick(d);require(d.done_o,"GROUP search timed out");}
int main(){
    Vmulti_config_group_selector d;d.rst_ni=0;d.start_i=0;tick(d);d.rst_ni=1;
    std::vector<unsigned> map(28,0);map[1]=1;map[4]=1;map[7]=1;map[16]=1;map[21]=1;
    set_map(d,map);run(d);
    require(d.success_o,"GROUP did not dominate EARLY on the same ConfigPatternMap");
    require((d.selected_configs_o&7U)==4,"GROUP did not select the row-releasing A configuration");
    require(d.combinations_checked_o==2401,"GROUP search did not cover 7^4 combinations");
    std::vector<unsigned> conflict(28,0);conflict[0]=1;conflict[9]=1;conflict[14]=1;conflict[21]=1;
    set_map(d,conflict);run(d);
    require(!d.success_o,"GROUP accepted conflicting use of D's physical row resource");
    std::cout<<"group_selector_test PASS\n";
}
