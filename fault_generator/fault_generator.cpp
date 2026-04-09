#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

/* ============ running command ==============
g++ -std=c++17 fault_generator.cpp -o fault_generator.o
./fault_generator.o --logic_units 4 --fixed_faults 12 --stack_height 2500
*/

struct Config
{
    int num_logic_units = 4;
    int stack_height = 2;
    int rows = 1024;
    int cols = 1024;
    double prob_cluster = 0.2;
    double prob_same_line = 0.3;
    int fixed_faults = 0;
    std::string output_file = "faults.faults";
    int seed = rand();
};

struct Fault { int r, c; };

int main(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--logic_units")    cfg.num_logic_units = std::stoi(argv[++i]);
        else if (arg == "--fixed_faults")   cfg.fixed_faults = std::stoi(argv[++i]);
        else if (arg == "--stack_height")   cfg.stack_height = std::stoi(argv[++i]);
        else if (arg == "--seed")           cfg.seed = std::stoi(argv[++i]);
    }

    std::mt19937 rng(cfg.seed);
    std::ofstream ofs(cfg.output_file);
    ofs << "// LogicUnitID LayerID BankID Row Col\n";

    std::uniform_int_distribution<> dr(0, cfg.rows - 1), dc(0, cfg.cols - 1), dn(-1, 1);
    std::uniform_real_distribution<> dp(0, 1);

    int pat_num = cfg.stack_height * cfg.num_logic_units;
    ofs << "" << pat_num << "\n";

    for (int ly = 0; ly < cfg.stack_height; ++ly) {
        for (int lu = 0; lu < cfg.num_logic_units; ++lu) {
            // randomize numbers of faults per logic unit per layer
            int rnd_fixed_faults = std::max(0, cfg.fixed_faults - 2) + (std::uniform_int_distribution<>(0, 4)(rng));
            ofs << rnd_fixed_faults << "\n";
            std::vector<Fault> bank_f;
            if (rnd_fixed_faults <= 0) continue;


            bank_f.push_back({dr(rng), dc(rng)});


            while (bank_f.size() < (size_t)rnd_fixed_faults) {
                double p = dp(rng);
                int nr, nc;


                const Fault& base = bank_f[std::uniform_int_distribution<>(0, bank_f.size() - 1)(rng)];

                if (p < cfg.prob_cluster) {
                    // Cluster Fault
                    nr = std::clamp(base.r + dn(rng), 0, cfg.rows - 1);
                    nc = std::clamp(base.c + dn(rng), 0, cfg.cols - 1);
                }
                else if (p < cfg.prob_cluster + cfg.prob_same_line) {
                    // Line Fault:
                    if (dp(rng) < 0.5) { nr = base.r; nc = dc(rng); }
                    else { nr = dr(rng); nc = base.c; }
                }
                else {
                    // Random Fault
                    nr = dr(rng); nc = dc(rng);
                }

                bool dup = false;
                for (auto& f : bank_f) if (f.r == nr && f.c == nc) { dup = true; break; }
                if (!dup) bank_f.push_back({nr, nc});
            }

            for (auto& f : bank_f) ofs << lu << " " << ly << " 0 " << f.r << " " << f.c << "\n";
        }
    }
    return 0;
}