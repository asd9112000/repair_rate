#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr std::size_t kExpectedGroupOnly = 4691;

enum class ResourceId { ARow = 0, DRow = 1, BColumn = 2, CColumn = 3 };

struct Decision
{
    int config = -1;
    int pattern = 0;
    int donor = -1;
    unsigned ledgerBefore = 0;
    unsigned ledgerAfter = 0;
};

struct RawPair
{
    std::string severity;
    int faultCount = 0;
    std::string imbalance;
    std::size_t vectorId = 0;
    std::string candidateMap;
    std::array<Decision, 4> early;
    std::array<Decision, 4> group;
    int earlyFailure = -1;
};

struct Effect
{
    bool moreRelease = false;
    bool lessBorrow = false;
    bool differentDonor = false;
    bool sameResourceEffect = false;
    std::string category;
    std::string criticalResource;
};

void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::string> parseCsv(const std::string &line)
{
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t index = 0; index < line.size(); ++index)
    {
        const char ch = line[index];
        if (ch == '"')
        {
            if (quoted && index + 1 < line.size() && line[index + 1] == '"')
            {
                field += ch;
                ++index;
            }
            else quoted = !quoted;
        }
        else if (ch == ',' && !quoted)
        {
            fields.push_back(field);
            field.clear();
        }
        else field += ch;
    }
    require(!quoted, "Unterminated quoted field in Phase 4J raw CSV");
    fields.push_back(field);
    return fields;
}

std::array<int, 4> parseArray(const std::string &text)
{
    std::array<int, 4> result{};
    std::istringstream input(text);
    std::string token;
    std::size_t index = 0;
    while (std::getline(input, token, ':'))
    {
        require(index < result.size(), "Unexpected array width in Phase 4J raw CSV");
        result[index++] = std::stoi(token);
    }
    require(index == result.size(), "Missing array value in Phase 4J raw CSV");
    return result;
}

bool needsRelease(std::size_t sa, int config)
{
    return config >= 0 && ((sa == 0 || sa == 3) ? (config == 4 || config == 6)
                                                   : (config == 1 || config == 3));
}

bool needsBorrow(std::size_t sa, int config)
{
    return config >= 0 && ((sa == 0 || sa == 3) ? (config == 5 || config == 6)
                                                   : (config == 2 || config == 3));
}

const char *saName(std::size_t sa)
{
    static constexpr std::array<const char *, 4> names{{"A", "B", "C", "D"}};
    return names.at(sa);
}

const char *resourceName(int resource)
{
    static constexpr std::array<const char *, 4> names{{"A_ROW", "D_ROW", "B_COL", "C_COL"}};
    return resource >= 0 && resource < static_cast<int>(names.size()) ? names[resource] : "NONE";
}

std::string roleName(std::size_t sa)
{
    return (sa == 0 || sa == 3) ? "A_D" : "B_C";
}

std::string configurationName(int config)
{
    return config < 0 ? "NONE" : "CFG" + std::to_string(config);
}

int firstDivergence(const RawPair &pair)
{
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        if (pair.early[sa].config != pair.group[sa].config ||
            pair.early[sa].pattern != pair.group[sa].pattern ||
            pair.early[sa].donor != pair.group[sa].donor)
            return static_cast<int>(sa);
    }
    return -1;
}

std::string criticalResource(const RawPair &pair, int failure)
{
    if (failure < 0 || failure > 3) return "NONE";
    const std::size_t sa = static_cast<std::size_t>(failure);
    const unsigned ledger = pair.early[sa].ledgerBefore;
    const auto unavailable = [&](ResourceId resource)
    {
        const std::size_t index = static_cast<std::size_t>(resource);
        const bool released = ((ledger >> index) & 1U) != 0;
        const unsigned borrower = (ledger >> (4 + 2 * index)) & 3U;
        return !released || borrower != 0;
    };
    const std::array<ResourceId, 2> donors = sa == 0 ? std::array<ResourceId, 2>{{ResourceId::BColumn, ResourceId::CColumn}}
        : sa == 1 ? std::array<ResourceId, 2>{{ResourceId::ARow, ResourceId::DRow}}
        : sa == 2 ? std::array<ResourceId, 2>{{ResourceId::DRow, ResourceId::ARow}}
                  : std::array<ResourceId, 2>{{ResourceId::CColumn, ResourceId::BColumn}};
    for (const ResourceId donor : donors)
        if (unavailable(donor)) return resourceName(static_cast<int>(donor));
    return "LOCAL_CANDIDATE_EXHAUSTION";
}

Effect classifyEffect(const RawPair &pair, int divergence)
{
    require(divergence >= 0 && divergence < 4, "GROUP_ONLY pair lacks a first divergence");
    const std::size_t sa = static_cast<std::size_t>(divergence);
    const Decision &early = pair.early[sa];
    const Decision &group = pair.group[sa];
    const bool earlyRelease = needsRelease(sa, early.config);
    const bool groupRelease = needsRelease(sa, group.config);
    const bool earlyBorrow = needsBorrow(sa, early.config);
    const bool groupBorrow = needsBorrow(sa, group.config);
    Effect result;
    result.moreRelease = groupRelease && !earlyRelease;
    result.lessBorrow = earlyBorrow && !groupBorrow;
    result.differentDonor = earlyBorrow && groupBorrow && early.donor != group.donor;
    const int effects = static_cast<int>(result.moreRelease) +
        static_cast<int>(result.lessBorrow) + static_cast<int>(result.differentDonor);
    result.sameResourceEffect = effects == 0 && earlyRelease == groupRelease &&
        earlyBorrow == groupBorrow;
    if (effects >= 2) result.category = "MULTIPLE_EFFECTS";
    else if (result.moreRelease) result.category = "GROUP_RELEASES_RESOURCE";
    else if (result.lessBorrow) result.category = "GROUP_AVOIDS_BORROW";
    else if (result.differentDonor) result.category = "GROUP_DIFFERENT_DONOR";
    else if (result.sameResourceEffect) result.category = "SAME_RESOURCE_EFFECT";
    else result.category = "OTHER_UNEXPLAINED";
    result.criticalResource = criticalResource(pair, pair.earlyFailure);
    return result;
}

RawPair decode(const std::vector<std::string> &fields)
{
    require(fields.size() == 21, "Phase 4J raw CSV column count changed");
    require(fields[5] == "GROUP_ONLY", "Phase 4K received a non-GROUP_ONLY row");
    RawPair pair;
    pair.severity = fields[0];
    pair.faultCount = std::stoi(fields[1]);
    pair.imbalance = fields[2];
    pair.vectorId = static_cast<std::size_t>(std::stoull(fields[3]));
    pair.candidateMap = fields[10];
    pair.earlyFailure = std::stoi(fields[8]);
    const auto earlyConfig = parseArray(fields[11]);
    const auto earlyPattern = parseArray(fields[12]);
    const auto earlyDonor = parseArray(fields[13]);
    const auto earlyBefore = parseArray(fields[14]);
    const auto earlyAfter = parseArray(fields[15]);
    const auto groupConfig = parseArray(fields[16]);
    const auto groupPattern = parseArray(fields[17]);
    const auto groupDonor = parseArray(fields[18]);
    const auto groupBefore = parseArray(fields[19]);
    const auto groupAfter = parseArray(fields[20]);
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        pair.early[sa] = {earlyConfig[sa], earlyPattern[sa], earlyDonor[sa],
                          static_cast<unsigned>(earlyBefore[sa]), static_cast<unsigned>(earlyAfter[sa])};
        pair.group[sa] = {groupConfig[sa], groupPattern[sa], groupDonor[sa],
                          static_cast<unsigned>(groupBefore[sa]), static_cast<unsigned>(groupAfter[sa])};
        // A failure makes the post-decision ledger equal to the pre-decision ledger;
        // Phase 4J deliberately records no selected transaction at that SA.
        if (pair.early[sa].config < 0) pair.early[sa].ledgerAfter = pair.early[sa].ledgerBefore;
    }
    return pair;
}

std::string compactTrace(const RawPair &pair)
{
    std::ostringstream output;
    for (std::size_t sa = 0; sa < 4; ++sa)
    {
        if (sa) output << " ; ";
        output << saName(sa) << ": E=" << configurationName(pair.early[sa].config)
               << "/P" << pair.early[sa].pattern << "/D" << resourceName(pair.early[sa].donor)
               << "/L" << pair.early[sa].ledgerAfter << " G="
               << configurationName(pair.group[sa].config) << "/P" << pair.group[sa].pattern
               << "/D" << resourceName(pair.group[sa].donor) << "/L"
               << pair.group[sa].ledgerAfter;
    }
    return output.str();
}

} // namespace

int main()
{
    try
    {
        std::ifstream raw("results/phase4j/phase4j_policy_tradeoff_raw.csv");
        require(raw.good(), "Phase 4J raw artifact is unavailable; do not regenerate a corpus");
        std::filesystem::create_directories("results/phase4k");
        std::ofstream classified("results/phase4k/phase4k_group_only_classification.csv");
        std::ofstream transitions("results/phase4k/phase4k_config_transition_matrix.csv");
        std::ofstream traces("results/phase4k/phase4k_representative_traces.csv");
        require(classified.good() && transitions.good() && traces.good(),
                "Unable to create Phase 4K artifacts");
        classified << "severity,fault_count,imbalance,vector_id,first_divergence_sa,"
                      "early_config,group_config,early_pattern,group_pattern,"
                      "early_release,group_release,early_donor,group_donor,"
                      "early_ledger_after_divergence,group_ledger_after_divergence,"
                      "early_failure_position,group_final_success,mechanism,"
                      "critical_later_resource\n";
        traces << "severity,fault_count,imbalance,vector_id,mechanism,"
                  "first_divergence_sa,candidate_valid_pattern_map,compact_sa_trace,"
                  "early_failure_position,critical_later_resource\n";

        std::string line;
        require(static_cast<bool>(std::getline(raw, line)), "Phase 4J raw CSV has no header");
        std::size_t groupOnly = 0;
        std::array<std::size_t, 4> divergence{};
        std::array<std::size_t, 4> failure{};
        std::map<std::pair<std::string, std::string>, std::size_t> transitionCounts;
        std::map<std::string, std::size_t> mechanismCounts;
        std::map<int, std::size_t> severityCounts;
        std::map<std::string, bool> representedTraceClass;
        std::size_t representativeCount = 0;
        std::set<std::string> ids;

        while (std::getline(raw, line))
        {
            const auto fields = parseCsv(line);
            require(fields.size() == 21, "Phase 4J raw CSV schema changed");
            if (fields[5] != "GROUP_ONLY") continue;
            const RawPair pair = decode(fields);
            const std::string id = pair.severity + ":" + std::to_string(pair.faultCount) +
                ":" + pair.imbalance + ":" + std::to_string(pair.vectorId);
            require(ids.insert(id).second, "Duplicate GROUP_ONLY vector ID in Phase 4J raw artifact");
            const int first = firstDivergence(pair);
            const Effect effect = classifyEffect(pair, first);
            require(pair.earlyFailure >= 0 && pair.earlyFailure < 4,
                    "GROUP_ONLY row has no valid EARLY failure position");
            ++groupOnly;
            ++divergence.at(static_cast<std::size_t>(first));
            ++failure.at(static_cast<std::size_t>(pair.earlyFailure));
            ++mechanismCounts[effect.category];
            ++severityCounts[pair.faultCount];
            transitionCounts[{roleName(static_cast<std::size_t>(first)),
                              configurationName(pair.early[first].config) + "->" +
                                  configurationName(pair.group[first].config)}]++;
            const Decision &early = pair.early[static_cast<std::size_t>(first)];
            const Decision &group = pair.group[static_cast<std::size_t>(first)];
            classified << pair.severity << ',' << pair.faultCount << ',' << pair.imbalance
                       << ',' << pair.vectorId << ',' << saName(static_cast<std::size_t>(first))
                       << ',' << configurationName(early.config) << ',' << configurationName(group.config)
                       << ',' << early.pattern << ',' << group.pattern << ','
                       << needsRelease(static_cast<std::size_t>(first), early.config) << ','
                       << needsRelease(static_cast<std::size_t>(first), group.config) << ','
                       << resourceName(early.donor) << ',' << resourceName(group.donor) << ','
                       << early.ledgerAfter << ',' << group.ledgerAfter << ','
                       << saName(static_cast<std::size_t>(pair.earlyFailure)) << ','
                       << "YES," << effect.category << ',' << effect.criticalResource << '\n';

            // The measured corpus has one mechanism but three distinct
            // downstream failure positions.  Preserve one actual trace for
            // each consequence rather than repeating equivalent D failures.
            const std::string traceClass = effect.category + ":" +
                std::to_string(pair.earlyFailure);
            if (representativeCount < 5 && !representedTraceClass[traceClass])
            {
                representedTraceClass[traceClass] = true;
                ++representativeCount;
                traces << pair.severity << ',' << pair.faultCount << ',' << pair.imbalance
                       << ',' << pair.vectorId << ',' << effect.category << ','
                       << saName(static_cast<std::size_t>(first)) << ",\""
                       << pair.candidateMap << "\",\"" << compactTrace(pair) << "\","
                       << saName(static_cast<std::size_t>(pair.earlyFailure)) << ','
                       << effect.criticalResource << '\n';
            }
        }

        require(groupOnly == kExpectedGroupOnly,
                "Phase 4K GROUP_ONLY accounting differs from accepted Phase 4J result");
        require(mechanismCounts["OTHER_UNEXPLAINED"] == 0,
                "An unexplained GROUP_ONLY mechanism requires investigation");
        transitions << "role,early_config,group_config,count\n";
        for (const auto &entry : transitionCounts)
        {
            const std::size_t arrow = entry.first.second.find("->");
            transitions << entry.first.first << ',' << entry.first.second.substr(0, arrow)
                        << ',' << entry.first.second.substr(arrow + 2) << ',' << entry.second << '\n';
        }

        const std::size_t proactive = divergence[0] + divergence[1] + divergence[2];
        std::cout << "PHASE4K_GROUP_ONLY_CASES=" << groupOnly << '\n';
        for (std::size_t sa = 0; sa < 4; ++sa)
        {
            std::cout << "PHASE4K_FIRST_DIVERGENCE_" << saName(sa) << '=' << divergence[sa] << '\n';
            std::cout << "PHASE4K_EARLY_FAILURE_" << saName(sa) << '=' << failure[sa] << '\n';
        }
        std::cout << "PHASE4K_GROUP_MORE_RELEASE=" << mechanismCounts["GROUP_RELEASES_RESOURCE"] << '\n'
                  << "PHASE4K_GROUP_LESS_BORROW=" << mechanismCounts["GROUP_AVOIDS_BORROW"] << '\n'
                  << "PHASE4K_GROUP_DIFFERENT_DONOR=" << mechanismCounts["GROUP_DIFFERENT_DONOR"] << '\n'
                  << "PHASE4K_GROUP_SAME_RESOURCE_EFFECT=" << mechanismCounts["SAME_RESOURCE_EFFECT"] << '\n'
                  << "PHASE4K_MULTI_EFFECT=" << mechanismCounts["MULTIPLE_EFFECTS"] << '\n'
                  << "PHASE4K_UNEXPLAINED_CASES=" << mechanismCounts["OTHER_UNEXPLAINED"] << '\n'
                  << "PHASE4K_DIVERGENCE_BEFORE_EARLY_FAILURE=" << proactive << '\n'
                  << "PHASE4K_TRACE_ACCOUNTING=PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "PHASE4K_GROUP_DOMINANCE_ANALYSIS FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
