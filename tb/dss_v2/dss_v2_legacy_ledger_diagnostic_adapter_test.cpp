#include "Vdss_v2_legacy_ledger_diagnostic_adapter.h"
#include <cstdlib>
#include <iostream>
void ck(Vdss_v2_legacy_ledger_diagnostic_adapter&d,unsigned rel,unsigned val,unsigned ids,unsigned out,bool ok){d.resource_released_i=rel;d.borrower_valid_i=val;d.borrower_id_flat_i=ids;d.eval();if(d.legacy_ledger_o!=out||bool(d.canonical_state_valid_o)!=ok)std::exit(1);}
int main(){Vdss_v2_legacy_ledger_diagnostic_adapter d;ck(d,0,0,0,0,1);ck(d,4,0,0,0x004,1);ck(d,4,4,0,0x104,1);ck(d,8,8,0,0x408,1);/* A_ROW borrowed B */ck(d,1,1,1,0x011,1);/* D_ROW borrowed C */ck(d,2,2,8,0x082,1);/* illegal A_ROW borrowed A */ck(d,1,1,0,0x031,0);
for(unsigned r=0;r<4;r++)for(unsigned id: (r<2?std::initializer_list<unsigned>{1,2}:std::initializer_list<unsigned>{0,3})){unsigned ids=id<<(2*r);ck(d,1u<<r,1u<<r,ids,(1u<<r)|((id==(r<2?1u:0u)?1u:2u)<<(4+2*r)),1);}std::cout<<"LEGACY_LEDGER_DIAGNOSTIC_PACK PASS\nROUND_TRIP PASS\n";}
