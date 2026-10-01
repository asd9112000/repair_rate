#include "Vrecam_n3_rc_group_selector.h"
#include "verilated.h"

#include "n3_rc_group_selector_oracle.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void clear_wide(WData* words, unsigned count) {
    for (unsigned index = 0; index < count; ++index) words[index] = 0U;
}

void put_bits(WData* words, unsigned bit, unsigned width, uint32_t value) {
    for (unsigned index = 0; index < width; ++index) {
        const unsigned target = bit + index;
        const WData mask = static_cast<WData>(1U) << (target & 31U);
        if ((value >> index) & 1U) words[target >> 5U] |= mask;
        else words[target >> 5U] &= ~mask;
    }
}

bool run_mask(Vrecam_n3_rc_group_selector& dut, unsigned valid_mask) {
    n3_rc_group_selector_oracle::CandidateStore store{};
    clear_wide(dut.candidate_store_image_i, 4U);
    for (unsigned sa = 0; sa < 4; ++sa)
        for (unsigned slot = 0; slot < 4; ++slot) {
            const unsigned record = sa * 4U + slot;
            const uint8_t pattern = static_cast<uint8_t>(record + 1U);
            store[sa][slot] = {((valid_mask >> record) & 1U) != 0U, pattern};
            put_bits(dut.candidate_store_image_i, record * 7U, 7U,
                     (static_cast<unsigned>(pattern) << 1U) | store[sa][slot].valid);
        }
    dut.eval();
    const auto expected = n3_rc_group_selector_oracle::select(store);
    const std::array<unsigned, 4> slots = {dut.selected_a_slot_o, dut.selected_b_slot_o,
                                           dut.selected_c_slot_o, dut.selected_d_slot_o};
    const std::array<unsigned, 4> configs = {dut.selected_a_config_id_o,
                                             dut.selected_b_config_id_o,
                                             dut.selected_c_config_id_o,
                                             dut.selected_d_config_id_o};
    const std::array<unsigned, 4> patterns = {dut.selected_a_pattern_id_o,
                                              dut.selected_b_pattern_id_o,
                                              dut.selected_c_pattern_id_o,
                                              dut.selected_d_pattern_id_o};
    if (dut.selected_valid_o != expected.repairable) return false;
    if (!expected.repairable) return true;
    for (unsigned sa = 0; sa < 4; ++sa)
        if (slots[sa] != expected.slot[sa] || configs[sa] != expected.config_id[sa] ||
            patterns[sa] != expected.pattern_id[sa])
            return false;
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vrecam_n3_rc_group_selector dut;
    unsigned legal_tuples = 0U;
    for (unsigned d = 0; d < 4; ++d)
        for (unsigned c = 0; c < 4; ++c)
            for (unsigned b = 0; b < 4; ++b)
                for (unsigned a = 0; a < 4; ++a)
                    legal_tuples += n3_rc_group_selector_oracle::resource_legal(
                        {static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                         static_cast<uint8_t>(c), static_cast<uint8_t>(d)});
    if (legal_tuples != 81U) return EXIT_FAILURE;
    for (unsigned mask = 0; mask < 65536U; ++mask)
        if (!run_mask(dut, mask)) {
            std::cerr << "selector mismatch valid_mask=" << mask << '\n';
            return EXIT_FAILURE;
        }
    const bool illegal_rejected =
        !n3_rc_group_selector_oracle::resource_legal({2U, 0U, 0U, 0U});
    if (!illegal_rejected) return EXIT_FAILURE;
    std::cout << "N3_RC_GROUP_SELECTOR_ORACLE_PASS valid_masks=65536 mismatches=0"
              << " legal_tuples=81 illegal_individual_combination_rejected=PASS\n";
    return EXIT_SUCCESS;
}
