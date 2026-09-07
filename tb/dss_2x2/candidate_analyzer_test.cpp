#include "Vcandidate_analyzer.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
using Fault = std::pair<unsigned, unsigned>;

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "candidate_analyzer_test: " << message << '\n';
        std::exit(1);
    }
}

void clearWords(WData *words, unsigned count)
{
    for (unsigned word = 0; word < count; ++word)
        words[word] = 0;
}

void setBits(WData *words, unsigned offset, unsigned width, unsigned value)
{
    for (unsigned bit = 0; bit < width; ++bit)
    {
        if ((value >> bit) & 1U)
            words[(offset + bit) / 32] |= 1U << ((offset + bit) % 32);
    }
}

unsigned getBits(const WData *words, unsigned offset, unsigned width)
{
    unsigned value = 0;
    for (unsigned bit = 0; bit < width; ++bit)
    {
        if ((words[(offset + bit) / 32] >> ((offset + bit) % 32)) & 1U)
            value |= 1U << bit;
    }
    return value;
}

void loadFaults(
    Vcandidate_analyzer &dut,
    const std::vector<Fault> &faults,
    unsigned activeRows,
    unsigned activeCols)
{
    clearWords(dut.fault_rows_flat_i, 4);
    clearWords(dut.fault_cols_flat_i, 4);
    dut.fault_valids_i = 0;
    for (unsigned index = 0; index < faults.size(); ++index)
    {
        dut.fault_valids_i |= 1U << index;
        setBits(dut.fault_rows_flat_i, index * 10, 10, faults[index].first);
        setBits(dut.fault_cols_flat_i, index * 10, 10, faults[index].second);
    }
    dut.active_rows_i = activeRows;
    dut.active_cols_i = activeCols;
    dut.eval();
}

void expectCandidate(
    const Vcandidate_analyzer &dut,
    unsigned candidate,
    unsigned rows,
    unsigned cols)
{
    require((dut.valid_candidates_o >> candidate) & 1U,
            "expected candidate is not valid");
    require(getBits(dut.used_rows_flat_o, candidate * 3, 3) == rows,
            "candidate row demand differs from the C++ golden model");
    require(getBits(dut.used_cols_flat_o, candidate * 3, 3) == cols,
            "candidate column demand differs from the C++ golden model");
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Vcandidate_analyzer dut;

    loadFaults(dut, {{10, 20}}, 2, 2);
    require(dut.repairable_o && dut.first_candidate_id_o == 0,
            "single fault should select candidate zero");
    expectCandidate(dut, 0, 1, 0);

    loadFaults(dut, {{7, 1}, {7, 2}, {7, 3}}, 2, 2);
    require(dut.repairable_o && dut.first_candidate_id_o == 0,
            "row-dominant trace did not select the mandatory row");
    expectCandidate(dut, 0, 1, 0);

    loadFaults(dut, {{1, 9}, {2, 9}, {3, 9}}, 2, 2);
    require(dut.repairable_o && dut.first_candidate_id_o == 3,
            "column-dominant trace did not select the mandatory column");
    expectCandidate(dut, 3, 0, 1);

    const std::array<std::vector<Fault>, 4> run23{{
        {{1004, 895}, {1005, 896}, {539, 110}, {662, 185},
         {662, 917}, {1005, 377}, {596, 538}, {386, 660}},
        {{14, 872}, {597, 872}, {991, 872}, {509, 444},
         {14, 46}, {849, 599}, {354, 878}},
        {{809, 792}, {789, 983}, {788, 982}, {789, 981},
         {1021, 982}, {789, 398}, {790, 981}},
        {{578, 135}, {606, 135}, {636, 517}, {378, 135},
         {636, 405}, {636, 411}}
    }};

    loadFaults(dut, run23[0], 2, 2);
    require(dut.valid_candidates_o == (1U << 4),
            "run23/A valid bitmap differs from C++");
    expectCandidate(dut, 4, 2, 2);

    loadFaults(dut, run23[1], 2, 2);
    require(!dut.repairable_o && dut.valid_candidates_o == 0,
            "run23/B local failure differs from C++");
    loadFaults(dut, run23[1], 3, 2);
    require(dut.valid_candidates_o == (1U << 6),
            "run23/B extended valid bitmap differs from C++");
    expectCandidate(dut, 6, 3, 2);

    loadFaults(dut, run23[2], 2, 2);
    require(dut.valid_candidates_o == ((1U << 0) | (1U << 4)),
            "run23/C valid bitmap differs from C++");
    expectCandidate(dut, 0, 2, 2);

    loadFaults(dut, run23[3], 2, 2);
    require(dut.valid_candidates_o == ((1U << 3) | (1U << 4)),
            "run23/D valid bitmap differs from C++");
    expectCandidate(dut, 3, 1, 1);

    std::cout << "candidate_analyzer_test PASS\n";
    return 0;
}
