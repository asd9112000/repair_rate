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
c
--fault_mode fixed   : every logic unit has exactly --fixed_faults faults
--fault_mode normal  : fault counts follow a normal distribution around --fixed_faults
--fault_mode extreme : fault counts tend to be either very low or high
*/

struct Config
{
    // ofs << "// HBM Channel Layer Bank SubarrayGroup Subarray Row Col";

    int fixed_faults = 10;
    double prob_cluster = 0.2;
    double prob_same_line = 0.3;

    int num_HBM = 1;
    int num_channel = 1;
    int stack_height = 1;
    int num_subarray_group = 1;
    int num_subarrays = 4;
    int rows = 1024;
    int cols = 1024;

    // std::string fault_mode = "fixed";
    // std::string fault_mode = "normal";
    std::string fault_mode = "extreme";
    std::string output_file = "faults.faults";
    int seed = rand();
};

struct Fault { int r, c; };

std::vector<int> faults_for_SAGroup(const Config& cfg, std::mt19937& rng) {
    std::vector<int> counts(cfg.num_subarrays, cfg.fixed_faults);
    if (cfg.fault_mode == "fixed" || cfg.fixed_faults == 0) {
        return counts;
    }

    std::vector<double> weights(cfg.num_subarrays);
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

    const int target_total = cfg.fixed_faults * cfg.num_subarrays;
    double weight_total = 0.0;
    for (double weight : weights) weight_total += weight;

    std::vector<std::pair<double, int>> fractions;
    fractions.reserve(cfg.num_subarrays);
    int assigned = 0;
    for (int i = 0; i < cfg.num_subarrays; ++i) {
        const double exact = weights[i] * target_total / weight_total;
        counts[i] = static_cast<int>(std::floor(exact));
        assigned += counts[i];
        fractions.push_back({exact - counts[i], i});
    }

    // Give the remaining faults to the largest fractional parts. This makes
    // every layer total exactly fixed_faults * num_subarrays.
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
            else if (arg == "--num_HBM")            cfg.num_HBM = std::stoi(argv[++i]);
            else if (arg == "--num_channel")        cfg.num_channel = std::stoi(argv[++i]);
            else if (arg == "--stack_height")       cfg.stack_height = std::stoi(argv[++i]);
            else if (arg == "--num_subarray_group") cfg.num_subarray_group = std::stoi(argv[++i]);
            else if (arg == "--num_subarrays")      cfg.num_subarrays = std::stoi(argv[++i]);
            else if (arg == "--rows")               cfg.rows = std::stoi(argv[++i]);
            else if (arg == "--cols")               cfg.cols = std::stoi(argv[++i]);
            else if (arg == "--fixed_faults")       cfg.fixed_faults = std::stoi(argv[++i]);
            else if (arg == "--seed")               cfg.seed = std::stoi(argv[++i]);
            else if (arg == "--fault_mode")         cfg.fault_mode = argv[++i];
            else if (arg == "--output")             cfg.output_file = argv[++i];
            else throw std::invalid_argument("unknown option: " + arg);
        }
        if (cfg.num_subarrays <= 0 || cfg.stack_height <= 0 || cfg.fixed_faults < 0) {
            throw std::invalid_argument("num_subarrays and stack_height must be positive; fixed_faults cannot be negative");
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
    ofs << "// HBMID ChannelID BankID SubarrayGroupID SubarrayID Row Col\n";
    ofs << "// Config: " << cfg.num_HBM << " HBM, "
        << cfg.num_channel << " channels, "
        << cfg.num_subarray_group << " subarray groups, "
        << cfg.num_subarrays << " logic units, "
        << cfg.stack_height << " layers, "
        << cfg.rows << " rows, "
        << cfg.cols << " cols, "
        << cfg.fixed_faults << " fixed faults, "
        << cfg.fault_mode << " fault mode, "
        << "seed=" << cfg.seed
        << "\n";

    std::uniform_int_distribution<> dr(0, cfg.rows - 1), dc(0, cfg.cols - 1), dn(-1, 1);
    std::uniform_real_distribution<> dp(0, 1);

    int pat_num = cfg.num_HBM * cfg.num_channel * cfg.stack_height
                * cfg.num_subarray_group * cfg.num_subarrays;
    ofs << "" << pat_num << "\n";

    for (int id_HBM = 0; id_HBM < cfg.num_HBM; ++id_HBM){
        for (int id_channel = 0; id_channel < cfg.num_channel; ++id_channel){
            for (int id_layer = 0; id_layer < cfg.stack_height; ++id_layer){
                for (int id_subarray_group = 0; id_subarray_group < cfg.num_subarray_group; ++id_subarray_group) {
                    const std::vector<int> layer_faults = faults_for_SAGroup(cfg, rng);

                    for (int id_sa = 0; id_sa < cfg.num_subarrays; ++id_sa) {
                        const int rnd_fixed_faults = layer_faults[id_sa];
                        ofs << rnd_fixed_faults << "\n";
                        std::vector<Fault> faults;
                        if (rnd_fixed_faults <= 0) continue;

                        faults.push_back({dr(rng), dc(rng)});

                        while (faults.size() < (size_t)rnd_fixed_faults) {
                            double p = dp(rng);
                            int nr, nc;


                            const Fault& base = faults[std::uniform_int_distribution<>(0, faults.size() - 1)(rng)];

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
                            for (auto& f : faults) if (f.r == nr && f.c == nc) { dup = true; break; }
                            if (!dup) faults.push_back({nr, nc});
                        }

                        // for (auto& f : faults) ofs << id_sa << " " << id_subarray_group << " 0 " << f.r << " " << f.c << "\n";
                        for (int i = 0; i < rnd_fixed_faults; ++i) {
                            ofs << id_HBM << " " << id_channel << " " << id_layer << " " << id_subarray_group << " " << id_sa << " " << faults[i].r << " " << faults[i].c << "\n";
                        }
                    }
                }
            }
        }
    }

    return 0;
}
