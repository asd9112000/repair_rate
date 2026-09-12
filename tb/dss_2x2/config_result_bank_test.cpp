#include "Vconfig_result_bank.h"
#include <cstdlib>
#include <iostream>
static void tick(Vconfig_result_bank &d){d.clk_i=0;d.eval();d.clk_i=1;d.eval();}
static void require(bool v,const char*m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
int main(){
    Vconfig_result_bank d;d.rst_ni=0;d.clear_i=0;d.capture_i=0;tick(d);d.rst_ni=1;
    d.subarray_i=2;d.config_patterns_i=0x07654321;d.capture_i=1;tick(d);d.capture_i=0;d.eval();
    require(d.subarray_valid_o==4,"captured subarray valid bit is wrong");
    const unsigned bit=2*28; const unsigned word=bit/32; const unsigned shift=bit%32;
    unsigned long long packed=d.config_pattern_map_o[word];
    if(shift>4) packed|=static_cast<unsigned long long>(d.config_pattern_map_o[word+1])<<32;
    require(((packed>>shift)&0x0fffffffULL)==0x07654321,"ConfigPatternMap did not retain exactly seven PatternIDs");
    std::cout<<"config_result_bank_test PASS\n";
}
