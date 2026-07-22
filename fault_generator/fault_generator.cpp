#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>
#include <stdexcept>

/* ============ running command ==============
g++ -std=c++17 fault_generator.cpp -o fault_generator.o
./fault_generator.o --logic_units 4 --fixed_faults 14 --stack_height 10 --fault_mode normal

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

std::vector<int> faults_for_layer(const Config& cfg, std::mt19937& rng) {
    std::vector<int> counts(cfg.num_logic_units, cfg.fixed_faults);
    if (cfg.fault_mode == "fixed" || cfg.fixed_faults == 0) {
        return counts;
    }

    std::vector<double> weights(cfg.num_logic_units);
    if (cfg.fault_mode == "normal") {
        // Small differences around fixed_faults.
        const double stddev = std::max(1.0, cfg.fixed_faults * 0.20);
        std::normal_distribution<double> distribution(cfg.fixed_faults, stddev);
        for (double& weight : weights) weight = std::max(0.0, distribution(rng));
    }
    else if (cfg.fault_mode == "extreme") {
        // Larger differences, while keeping the layer's total unchanged.
        std::lognormal_distribution<double> distribution(0.0, 0.65);
        for (double& weight : weights) weight = distribution(rng);
    }

    const int target_total = cfg.fixed_faults * cfg.num_logic_units;
    double weight_total = 0.0;
    for (double weight : weights) weight_total += weight;

    std::vector<std::pair<double, int>> fractions;
    fractions.reserve(cfg.num_logic_units);
    int assigned = 0;
    for (int i = 0; i < cfg.num_logic_units; ++i) {
        const double exact = weights[i] * target_total / weight_total;
        counts[i] = static_cast<int>(std::floor(exact));
        assigned += counts[i];
        fractions.push_back({exact - counts[i], i});
    }

    // Give the remaining faults to the largest fractional parts. This makes
    // every layer total exactly fixed_faults * num_logic_units.
    std::sort(fractions.begin(), fractions.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    for (int i = 0; assigned < target_total; ++i, ++assigned) {
        ++counts[fractions[i].second];
    }
    return counts;
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
        const std::vector<int> layer_faults = faults_for_layer(cfg, rng);
        for (int lu = 0; lu < cfg.num_logic_units; ++lu) {
            const int rnd_fixed_faults = layer_faults[lu];
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
