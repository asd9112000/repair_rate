# 1x4 RS2/CS2/m1 candidate producer

The producer captures ten requests with one shared analyzer in order:
`A0,A1,B0,B1,B2,C0,C1,C2,D0,D1`. It stores PatternID-valid bits and actual
`usedRows`/`usedColumns` for every 15-slot attempt region.

```wavedrom
{signal:[{name:'start_i',wave:'010'},{name:'active_sa_o',wave:'=2345',data:'A B C D'},{name:'active_attempt_o',wave:'=3456789',data:'0 1 0 1 2 0 1'},{name:'done_o',wave:'0.........10'}]}
```
