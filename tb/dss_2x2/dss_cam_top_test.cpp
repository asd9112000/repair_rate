#include "Vdss_cam_top.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
struct Fault
{
    unsigned subarray;
    unsigned row;
    unsigned column;
};

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        std::cerr << "dss_cam_top_test: " << message << '\n';
        std::exit(1);
    }
}

void tick(Vdss_cam_top &dut)
{
    dut.clk_i = 0;
    dut.eval();
    dut.clk_i = 1;
    dut.eval();
    dut.clk_i = 0;
    dut.eval();
}

unsigned field(std::uint32_t packed, unsigned index, unsigned width)
{
    return (packed >> (index * width)) & ((1U << width) - 1U);
}

void clearGroup(Vdss_cam_top &dut)
{
    dut.clear_i = 1;
    tick(dut);
    dut.clear_i = 0;
    dut.eval();
    require(dut.fault_ready_o, "collector did not become ready after clear");
}

void pushFault(Vdss_cam_top &dut, const Fault &fault)
{
    require(dut.fault_ready_o, "collector backpressured a legal fault");
    dut.fault_subarray_i = fault.subarray;
    dut.fault_row_i = fault.row;
    dut.fault_col_i = fault.column;
    dut.fault_valid_i = 1;
    tick(dut);
    dut.fault_valid_i = 0;
    dut.eval();
}

void analyze(Vdss_cam_top &dut)
{
    dut.analysis_start_i = 1;
    tick(dut);
    dut.analysis_start_i = 0;
    for (unsigned cycle = 0; cycle < 66000 && !dut.analysis_done_o; ++cycle)
        tick(dut);
    require(dut.analysis_done_o, "analysis did not finish within the bounded search space");
    require(dut.analysis_cycles_o == 65536,
            "cycle counter does not equal the 16^4 demand-combination search");
}

void runCase(
    Vdss_cam_top &dut,
    const std::string &name,
    const std::vector<Fault> &faults,
    bool expectedRepairable,
    const std::array<unsigned, 4> &expectedAttempts,
    const std::array<unsigned, 4> &expectedCandidates,
    const std::array<unsigned, 4> &expectedRows,
    const std::array<unsigned, 4> &expectedCols,
    unsigned expectedBorrowedRows,
    unsigned expectedBorrowedCols)
{
    clearGroup(dut);
    for (const Fault &fault : faults)
        pushFault(dut, fault);
    analyze(dut);
    require(static_cast<bool>(dut.repairable_o) == expectedRepairable,
            name + ": repairable result differs from golden");
    require(dut.borrowed_rows_o == expectedBorrowedRows &&
                dut.borrowed_cols_o == expectedBorrowedCols,
            name + ": borrowed resource count differs from golden");
    if (expectedRepairable)
    {
        for (unsigned sa = 0; sa < 4; ++sa)
        {
            require(field(dut.selected_attempts_o, sa, 1) == expectedAttempts[sa],
                    name + ": selected attempt differs from golden");
            require(field(dut.selected_candidates_o, sa, 5) == expectedCandidates[sa],
                    name + ": selected candidate differs from golden");
            require(field(dut.selected_rows_o, sa, 3) == expectedRows[sa],
                    name + ": selected row demand differs from golden");
            require(field(dut.selected_cols_o, sa, 3) == expectedCols[sa],
                    name + ": selected column demand differs from golden");
        }
    }
    std::cout << name << " PASS\n";
}
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    const std::string selectedCase = argc > 1 ? argv[1] : "";
    const auto shouldRun = [&selectedCase](const std::string &name) {
        return selectedCase.empty() || selectedCase == name;
    };
    bool ranCase = false;
    Vdss_cam_top dut;
    dut.rst_ni = 0;
    dut.clear_i = 0;
    dut.fault_valid_i = 0;
    dut.analysis_start_i = 0;
    tick(dut);
    dut.rst_ni = 1;

    const std::array<unsigned, 4> zeros{{0, 0, 0, 0}};
    if (shouldRun("normal_empty")) {
        ranCase = true;
        runCase(dut, "normal_empty", {}, true, zeros, zeros, zeros, zeros, 0, 0);
    }
    if (shouldRun("single_fault")) {
        ranCase = true;
        runCase(dut, "single_fault", {{0, 10, 20}}, true,
            zeros, zeros, {{1, 0, 0, 0}}, zeros, 0, 0);
    }
    if (shouldRun("row_dominant")) {
        ranCase = true;
        runCase(dut, "row_dominant", {{0, 7, 1}, {0, 7, 2}, {0, 7, 3}}, true,
            zeros, zeros, {{1, 0, 0, 0}}, zeros, 0, 0);
    }
    if (shouldRun("column_dominant")) {
        ranCase = true;
        runCase(dut, "column_dominant", {{1, 1, 9}, {1, 2, 9}, {1, 3, 9}}, true,
            zeros, {{0, 3, 0, 0}}, zeros, {{0, 1, 0, 0}}, 0, 0);
    }

    std::vector<Fault> sharingRequired;
    for (unsigned index = 0; index < 9; ++index)
        sharingRequired.push_back({0, 100 + index, 200 + index});
    if (shouldRun("sharing_required")) {
        ranCase = true;
        runCase(dut, "sharing_required", sharingRequired, true,
            {{1, 0, 0, 0}}, zeros,
            {{2, 0, 0, 0}}, {{3, 0, 0, 0}}, 0, 1);
    }

    std::vector<Fault> overflow;
    for (unsigned index = 0; index < 10; ++index)
        overflow.push_back({0, 300 + index, 500 + index});
    if (shouldRun("unrepairable_overflow")) {
        ranCase = true;
        runCase(dut, "unrepairable_overflow", overflow, false,
            zeros, zeros, zeros, zeros, 0, 0);
    }

    std::vector<Fault> donorBoundary = sharingRequired;
    donorBoundary.insert(donorBoundary.end(), {
        {1, 1, 10}, {1, 2, 10}, {1, 3, 10},
        {1, 4, 20}, {1, 5, 20}, {1, 6, 20}});
    if (shouldRun("boundary_donor_busy")) {
        ranCase = true;
        runCase(dut, "boundary_donor_busy", donorBoundary, false,
            zeros, zeros, zeros, zeros, 0, 0);
    }

    const std::array<std::vector<std::pair<unsigned, unsigned>>, 4> run23{{
        {{{1004, 895}, {1005, 896}, {539, 110}, {662, 185},
          {662, 917}, {1005, 377}, {596, 538}, {386, 660}}},
        {{{14, 872}, {597, 872}, {991, 872}, {509, 444},
          {14, 46}, {849, 599}, {354, 878}}},
        {{{809, 792}, {789, 983}, {788, 982}, {789, 981},
          {1021, 982}, {789, 398}, {790, 981}}},
        {{{578, 135}, {606, 135}, {636, 517}, {378, 135},
          {636, 405}, {636, 411}}}
    }};
    std::vector<Fault> run23Faults;
    for (unsigned sa = 0; sa < 4; ++sa)
        for (const auto &address : run23[sa])
            run23Faults.push_back({sa, address.first, address.second});
    if (shouldRun("cpp_run23")) {
        ranCase = true;
        runCase(dut, "cpp_run23", run23Faults, true,
            {{0, 1, 0, 0}}, {{4, 6, 0, 3}},
            {{2, 3, 2, 1}}, {{2, 2, 2, 1}}, 1, 0);
    }

    require(ranCase, "unknown test case: " + selectedCase);

    std::cout << "dss_cam_top_test PASS";
    if (!selectedCase.empty())
        std::cout << " (" << selectedCase << ")";
    std::cout << '\n';
    return 0;
}
