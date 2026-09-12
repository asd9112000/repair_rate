#include "Vrecam_dss_group_allocator.h"
#include <cstdint>
#include <iostream>
static void put(Vrecam_dss_group_allocator&d,int sa,int i,int cfg){uint32_t*p=sa==0?d.map_a_i:sa==1?d.map_b_i:sa==2?d.map_c_i:d.map_d_i;unsigned b=i*20,w=b/32,o=b%32;std::uint64_t x=(1ull<<5)|(1ull<<4)|(std::uint64_t(cfg)<<1)|1;p[w]|=x<<o;if(o>12)p[w+1]|=x>>(32-o);}static void tick(Vrecam_dss_group_allocator&d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
int main(){Vrecam_dss_group_allocator d;d.rst_ni=0;d.start_i=0;d.policy_i=0;for(auto&w:d.map_a_i)w=0;for(auto&w:d.map_b_i)w=0;for(auto&w:d.map_c_i)w=0;for(auto&w:d.map_d_i)w=0;tick(d);d.rst_ni=1;put(d,0,0,0);put(d,0,1,4);put(d,1,1,2);put(d,2,0,0);put(d,3,0,0);d.policy_i=1;d.start_i=1;tick(d);d.start_i=0;for(int i=0;i<4;i++)tick(d);if(!d.done_o||!d.group_repairable_o||d.sa_commit_valid_o!=15)return 1;if((d.ledger_released_borrower_o&1)==0||((d.ledger_released_borrower_o>>4)&3)!=1)return 1;std::cout<<"PHASE3HI_GROUP_REGRESSION PASS\n";}
