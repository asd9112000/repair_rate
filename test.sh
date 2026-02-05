
#!/bin/bash

rm main.log
# 執行 30 次
./fault_generator/fault_generator.o --logic_units 1 --fixed_faults 12
for i in {1..30}
do
    echo "執行第 $i 次" >> main.log
    ./main.o 2 2 >> main.log
done