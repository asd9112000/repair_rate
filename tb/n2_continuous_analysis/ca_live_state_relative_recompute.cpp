#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Row {
    unsigned case_id = 0;
    std::string policy;
    std::string outcome;
    unsigned rank = 0;
    unsigned latency = 0;
    unsigned g_zero = 0;
    std::string state_image;
};

struct Stats {
    unsigned n = 0;
    double mean = 0.0;
    unsigned median = 0;
    unsigned p95 = 0;
    unsigned max = 0;
};

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) fields.push_back(field);
    return fields;
}

Stats stats(std::vector<unsigned> values) {
    std::sort(values.begin(), values.end());
    Stats result;
    result.n = static_cast<unsigned>(values.size());
    if (values.empty()) return result;
    unsigned long long sum = 0;
    for (unsigned value : values) sum += value;
    result.mean = static_cast<double>(sum) / values.size();
    result.median = values[(values.size() - 1) / 2];
    result.p95 = values[(values.size() - 1) * 95 / 100];
    result.max = values.back();
    return result;
}

void write_stats(std::ofstream& output, const std::string& policy, unsigned gap,
                 const std::vector<unsigned>& values) {
    const Stats value_stats = stats(values);
    unsigned zero_count = 0;
    for (unsigned value : values) if (value == 0) ++zero_count;
    output << policy << ',' << gap << ',' << value_stats.n << ',' << std::fixed
           << std::setprecision(6) << value_stats.mean << ',' << value_stats.median << ','
           << value_stats.p95 << ',' << value_stats.max << ',' << zero_count << ','
           << (value_stats.n == 0 ? 0.0 : static_cast<double>(zero_count) / value_stats.n) << '\n';
}

bool load_rows(const std::string& path, std::vector<Row>& rows) {
    std::ifstream input(path);
    std::string line;
    if (!std::getline(input, line)) return false;
    while (std::getline(input, line)) {
        const auto fields = split(line);
        if (fields.size() != 12) return false;
        Row row;
        row.case_id = static_cast<unsigned>(std::stoul(fields[0]));
        row.policy = fields[1];
        row.outcome = fields[3];
        row.rank = static_cast<unsigned>(std::stoul(fields[4]));
        row.latency = static_cast<unsigned>(std::stoul(fields[9]));
        row.g_zero = static_cast<unsigned>(std::stoul(fields[10]));
        row.state_image = fields[11];
        rows.push_back(row);
    }
    return !rows.empty();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: RAW_CSV SUMMARY_DIR\n";
        return 2;
    }
    std::vector<Row> rows;
    if (!load_rows(argv[1], rows)) {
        std::cerr << "invalid raw CSV\n";
        return 1;
    }
    std::filesystem::create_directories(argv[2]);
    const std::filesystem::path summary_dir(argv[2]);
    std::ofstream sweep(summary_dir / "CA_LIVE_STATE_RELATIVE_SWEEP.csv");
    std::ofstream g_zero(summary_dir / "CA_LIVE_STATE_RELATIVE_G_ZERO.csv");
    std::ofstream paired(summary_dir / "CA_LIVE_STATE_RELATIVE_PAIRED.csv");
    if (!sweep || !g_zero || !paired) return 2;

    sweep << "policy,G_state,N,mean_L_post_state,median_L_post_state,p95_L_post_state,"
             "max_L_post_state,zero_latency_count,zero_latency_fraction\n";
    g_zero << "policy,N,min_G_state_zero,mean_G_state_zero,median_G_state_zero,"
              "p95_G_state_zero,max_G_state_zero\n";
    paired << "metric,G_state,N,min,mean,median,p95,max,early_less_group,equal,early_greater_group\n";

    std::map<std::string, std::vector<unsigned>> latency;
    std::map<std::string, std::vector<unsigned>> zero;
    std::vector<unsigned> early_selected_legal;
    std::vector<unsigned> early_unrepairable_full_search;
    std::map<unsigned, std::map<std::string, Row>> pairs;
    for (const Row& row : rows) {
        latency[row.policy].push_back(row.latency);
        zero[row.policy].push_back(row.g_zero);
        if (row.policy == "EARLY" && row.outcome == "REPAIRABLE")
            early_selected_legal.push_back(row.latency);
        if (row.policy == "EARLY" && row.outcome == "UNREPAIRABLE_FULL_SEARCH")
            early_unrepairable_full_search.push_back(row.latency);
        pairs[row.case_id][row.policy] = row;
    }
    if (latency["EARLY"].size() != latency["GROUP"].size()) return 1;

    for (const auto& entry : latency) {
        const Stats value_stats = stats(zero[entry.first]);
        const unsigned minimum = *std::min_element(zero[entry.first].begin(), zero[entry.first].end());
        g_zero << entry.first << ',' << value_stats.n << ',' << minimum << ',' << std::fixed
               << std::setprecision(6) << value_stats.mean << ',' << value_stats.median << ','
               << value_stats.p95 << ',' << value_stats.max << '\n';
        for (unsigned gap = 0; gap <= 8; ++gap) {
            std::vector<unsigned> exposed;
            for (unsigned policy_latency : entry.second)
                exposed.push_back(policy_latency > gap ? policy_latency - gap : 0U);
            write_stats(sweep, entry.first, gap, exposed);
        }
    }
    for (const auto& entry : std::vector<std::pair<std::string, std::vector<unsigned>>>{
             {"EARLY_SELECTED_LEGAL", early_selected_legal},
             {"EARLY_UNREPAIRABLE_FULL_SEARCH", early_unrepairable_full_search}}) {
        const Stats value_stats = stats(entry.second);
        const unsigned minimum = *std::min_element(entry.second.begin(), entry.second.end());
        g_zero << entry.first << ',' << value_stats.n << ',' << minimum << ',' << std::fixed
               << std::setprecision(6) << value_stats.mean << ',' << value_stats.median << ','
               << value_stats.p95 << ',' << value_stats.max << '\n';
    }

    std::vector<unsigned> deltas;
    for (const auto& entry : pairs) {
        const auto early = entry.second.find("EARLY");
        const auto group = entry.second.find("GROUP");
        if (early == entry.second.end() || group == entry.second.end()) return 1;
        if (early->second.state_image != group->second.state_image) return 1;
        deltas.push_back(group->second.g_zero - early->second.g_zero);
    }
    const Stats delta_stats = stats(deltas);
    paired << "GROUP_MINUS_EARLY_G_STATE_ZERO,-1," << delta_stats.n << ','
           << *std::min_element(deltas.begin(), deltas.end()) << ',' << std::fixed
           << std::setprecision(6) << delta_stats.mean << ',' << delta_stats.median << ','
           << delta_stats.p95 << ',' << delta_stats.max << ",,,\n";
    for (unsigned gap = 0; gap <= 8; ++gap) {
        unsigned early_less = 0;
        unsigned equal = 0;
        unsigned early_greater = 0;
        for (const auto& entry : pairs) {
            const unsigned early_exposed = entry.second.at("EARLY").latency > gap
                ? entry.second.at("EARLY").latency - gap : 0U;
            const unsigned group_exposed = entry.second.at("GROUP").latency > gap
                ? entry.second.at("GROUP").latency - gap : 0U;
            if (early_exposed < group_exposed) ++early_less;
            else if (early_exposed == group_exposed) ++equal;
            else ++early_greater;
        }
        paired << "L_POST_STATE_COMPARISON," << gap << ',' << pairs.size()
               << ",,,,,," << early_less << ',' << equal << ',' << early_greater << '\n';
    }
    std::cout << "SUMMARY_RECOMPUTE_MATCH=PASS rows=" << rows.size()
              << " paired_cases=" << pairs.size() << '\n';
    return 0;
}
