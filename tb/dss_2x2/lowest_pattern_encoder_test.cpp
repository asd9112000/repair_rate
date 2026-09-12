#include "Vlowest_pattern_encoder.h"
#include <cstdlib>
#include <iostream>
static void require(bool v, const char *m) { if (!v) { std::cerr << m << '\n'; std::exit(1); } }
int main() {
    Vlowest_pattern_encoder d;
    d.candidate_valid_i = 0; d.eval();
    require(!d.config_valid_o && d.pattern_id_o == 0, "empty candidate set was not invalid");
    d.candidate_valid_i = (1U << 6) | (1U << 3) | (1U << 1); d.eval();
    require(d.config_valid_o && d.pattern_id_o == 2, "encoder did not retain lowest one-based PatternID");
    std::cout << "lowest_pattern_encoder_test PASS\n";
}
