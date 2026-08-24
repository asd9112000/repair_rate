#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>
#include <stdexcept>

/*
fault_generator_rand_amount.cpp: Generates a fault file with randomly distributed faults .
fault_generator.cpp: Generates a fault file with a fixed number of faults per subarray group.
*/

/* ============ running command ==============
# From the repository root:
make fault_generator_rand_b
./build/bin/fault_generator_rand_amount --fixed_faults 10 --stack_height 100 --fault_mode normal --output fault_generator/faults.faults

--fault_mode fixed   : every logic unit has exactly --fixed_faults faults
--fault_mode normal  : fault counts follow a normal distribution around --fixed_faults
--fault_mode extreme : fault counts tend to be either very low or high
*/

struct Config
{
    int num_logic_units = 4;
    int stack_height = 2;
    int rows = 1024;
    int cols = 1024;
    double prob_cluster = 0.2;
    double prob_same_line = 0.3;
    int fixed_faults = 10;
    // std::string fault_mode = "fixed";
    // std::string fault_mode = "normal";
    std::string fault_mode = "extreme";
    std::string output_file = "faults.faults";
    int seed = rand();
};

struct Fault { int r, c; };

int faults_for_logic_unit(const Config& cfg, std::mt19937& rng) {
    if (cfg.fault_mode == "fixed" || cfg.fixed_faults == 0) {
        return cfg.fixed_faults;
    }

    if (cfg.fault_mode == "normal") {
        // About 68% of values are within +/-20% of fixed_faults.
        const double stddev = std::max(1.0, cfg.fixed_faults * 0.40);
        std::normal_distribution<double> distribution(cfg.fixed_faults, stddev);
        return std::max(0, static_cast<int>(std::lround(distribution(rng))));
    }

    if (cfg.fault_mode == "extreme") {
        // A widely separated bimodal distribution: half of the units have
        // 0% to 20% of fixed_faults, while the other half have 150% to 200%.
        std::bernoulli_distribution choose_low(0.7);
        if (choose_low(rng)) {
            std::uniform_int_distribution<int> low(
                0, static_cast<int>(std::floor(cfg.fixed_faults * 0.20)));
            return low(rng);
        }
        std::uniform_int_distribution<int> high(
            static_cast<int>(std::ceil(cfg.fixed_faults * 1.20)),
            static_cast<int>(std::ceil(cfg.fixed_faults * 1.40)));
        return high(rng);
    }

    throw std::invalid_argument("unknown fault mode: " + cfg.fault_mode);
}

int main(int argc, char* argv[]) {
    Config cfg;
    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
            if      (arg == "--logic_units")    cfg.num_logic_units = std::stoi(argv[++i]);
            else if (arg == "--fixed_faults")   cfg.fixed_faults = std::stoi(argv[++i]);
            else if (arg == "--stack_height")   cfg.stack_height = std::stoi(argv[++i]);
            else if (arg == "--seed")           cfg.seed = std::stoi(argv[++i]);
            else if (arg == "--fault_mode")     cfg.fault_mode = argv[++i];
            else if (arg == "--output")         cfg.output_file = argv[++i];
            else throw std::invalid_argument("unknown option: " + arg);
        }
        if (cfg.num_logic_units <= 0 || cfg.stack_height <= 0 || cfg.fixed_faults < 0) {
            throw std::invalid_argument("logic_units and stack_height must be positive; fixed_faults cannot be negative");
        }
        if (cfg.fault_mode != "fixed" && cfg.fault_mode != "normal" && cfg.fault_mode != "extreme") {
            throw std::invalid_argument("fault_mode must be fixed, normal, or extreme");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::mt19937 rng(cfg.seed);
    std::ofstream ofs(cfg.output_file);
    if (!ofs) {
        std::cerr << "Error: cannot open output file: " << cfg.output_file << "\n";
        return 1;
    }
    ofs << "// LogicUnitID LayerID BankID Row Col\n";
    ofs << "// Config: " << cfg.num_logic_units << " logic units, "
        << cfg.stack_height << " layers, "
        << cfg.rows << " rows, "
        << cfg.cols << " cols, "
        << cfg.fixed_faults << " fixed faults, "
        << cfg.fault_mode << " fault mode, "
        << "seed=" << cfg.seed
        << "\n";
    std::uniform_int_distribution<> dr(0, cfg.rows - 1), dc(0, cfg.cols - 1), dn(-1, 1);
    std::uniform_real_distribution<> dp(0, 1);

    int pat_num = cfg.stack_height * cfg.num_logic_units;
    ofs << "" << pat_num << "\n";

    for (int ly = 0; ly < cfg.stack_height; ++ly) {
        for (int lu = 0; lu < cfg.num_logic_units; ++lu) {
            const int rnd_fixed_faults = faults_for_logic_unit(cfg, rng);
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
