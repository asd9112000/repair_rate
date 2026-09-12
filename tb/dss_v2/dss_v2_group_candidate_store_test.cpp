#include "Vdss_v2_group_candidate_store_test_top.h"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void expect(bool value, const char* name) {
    if (!value) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
}
void tick(Vdss_v2_group_candidate_store_test_top& dut) {
    dut.clk_i = 0; dut.eval(); dut.clk_i = 1; dut.eval();
}
void read(Vdss_v2_group_candidate_store_test_top& dut, unsigned sa, unsigned slot,
          bool valid, unsigned pattern, const char* name) {
    dut.read_sa_i = sa; dut.read_slot_i = slot; dut.eval();
    expect(bool(dut.read_candidate_valid_o) == valid, name);
    if (valid) expect(dut.read_pattern_id_o == pattern, name);
    else expect(dut.read_pattern_id_o == 0, name);
}
void write(Vdss_v2_group_candidate_store_test_top& dut, unsigned sa, unsigned slot,
           bool valid, unsigned pattern) {
    dut.write_enable_i = 1; dut.write_sa_i = sa; dut.write_slot_i = slot;
    dut.write_candidate_valid_i = valid; dut.write_pattern_id_i = pattern; tick(dut);
    dut.write_enable_i = 0;
}
unsigned expected_config(unsigned sa, unsigned slot) {
    static constexpr unsigned table[4][4] = {{0,4,5,6},{0,1,2,3},{0,1,2,3},{0,4,5,6}};
    return table[sa][slot];
}
unsigned expected_descriptor(unsigned config) {
    static constexpr unsigned descriptor[7] = {
        (2 << 3) | 2, (2 << 3) | 1, (3 << 3) | 2, (3 << 3) | 1,
        (1 << 3) | 2, (2 << 3) | 3, (1 << 3) | 3};
    return descriptor[config];
}
}  // namespace

int main() {
    Vdss_v2_group_candidate_store_test_top dut;
    dut.rst_ni = 0; dut.write_enable_i = 0; dut.write_sa_i = dut.write_slot_i = 0;
    dut.write_candidate_valid_i = 0; dut.write_pattern_id_i = 0;
    dut.read_sa_i = dut.read_slot_i = 0; dut.decode_sa_i = dut.decode_slot_i = 0;
    dut.priority_sa_i = 0; tick(dut); dut.rst_ni = 1;

    expect(dut.candidate_store_image_o[0] == 0 && dut.candidate_store_image_o[1] == 0 &&
           dut.candidate_store_image_o[2] == 0, "RESET_EMPTY");
    std::cout << "RESET_EMPTY " << (failures ? "FAIL" : "PASS") << '\n';

    write(dut, 0, 1, true, 0xa);
    read(dut, 0, 1, true, 0xa, "WRITE_SINGLE_CANDIDATE");
    std::cout << "WRITE_SINGLE_CANDIDATE " << (failures ? "FAIL" : "PASS") << '\n';

    dut.rst_ni = 0; tick(dut); dut.rst_ni = 1;
    for (unsigned sa = 0; sa != 4; ++sa)
        for (unsigned slot = 0; slot != 4; ++slot)
            write(dut, sa, slot, true, sa * 4 + slot);
    for (unsigned sa = 0; sa != 4; ++sa)
        for (unsigned slot = 0; slot != 4; ++slot)
            read(dut, sa, slot, true, sa * 4 + slot, "WRITE_ALL_16_CANDIDATES");
    std::cout << "WRITE_ALL_16_CANDIDATES " << (failures ? "FAIL" : "PASS") << '\n';

    write(dut, 2, 3, true, 0x5);
    read(dut, 2, 3, true, 0x5, "OVERWRITE_SAME_SLOT");
    std::cout << "OVERWRITE_SAME_SLOT " << (failures ? "FAIL" : "PASS") << '\n';

    write(dut, 1, 2, false, 0xf);
    read(dut, 1, 2, false, 0, "INVALID_CANDIDATE_VALID_BIT");
    std::cout << "INVALID_CANDIDATE_VALID_BIT " << (failures ? "FAIL" : "PASS") << '\n';

    write(dut, 3, 0, true, 0xd);
    read(dut, 3, 0, true, 0xd, "PATTERN_ID_STORAGE");
    std::cout << "PATTERN_ID_STORAGE " << (failures ? "FAIL" : "PASS") << '\n';

    dut.rst_ni = 0; tick(dut); dut.rst_ni = 1;
    write(dut, 0, 0, true, 1); write(dut, 1, 0, true, 2);
    read(dut, 0, 0, true, 1, "SA_ISOLATION");
    read(dut, 1, 0, true, 2, "SA_ISOLATION");
    read(dut, 2, 0, false, 0, "SA_ISOLATION");
    std::cout << "SA_ISOLATION " << (failures ? "FAIL" : "PASS") << '\n';

    write(dut, 0, 2, true, 7);
    read(dut, 0, 0, true, 1, "SLOT_ISOLATION");
    read(dut, 0, 1, false, 0, "SLOT_ISOLATION");
    read(dut, 0, 2, true, 7, "SLOT_ISOLATION");
    std::cout << "SLOT_ISOLATION " << (failures ? "FAIL" : "PASS") << '\n';

    for (unsigned sa = 0; sa != 4; ++sa) for (unsigned slot = 0; slot != 4; ++slot) {
        dut.decode_sa_i = sa; dut.decode_slot_i = slot; dut.eval();
        const unsigned config = expected_config(sa, slot);
        expect(dut.decoded_config_id_o == config, "CANONICAL_CONFIG_DECODE");
        expect(dut.decoded_descriptor_o == expected_descriptor(config), "CANONICAL_CONFIG_DECODE");
    }
    std::cout << "CANONICAL_CONFIG_DECODE " << (failures ? "FAIL" : "PASS") << '\n';

    dut.rst_ni = 0; tick(dut); dut.rst_ni = 1;
    for (unsigned slot = 0; slot != 4; ++slot) write(dut, 2, slot, true, 8 + slot);
    dut.priority_sa_i = 2; dut.eval();
    const unsigned expected_slots[] = {1, 0, 3, 2};
    for (unsigned rank = 0; rank != 4; ++rank) {
        expect(((dut.priority_slot_flat_o >> (rank * 2)) & 3) == expected_slots[rank],
               "GROUP_SLOT_PRIORITY_ORDER");
        expect((dut.priority_valid_o >> rank) & 1, "GROUP_SLOT_PRIORITY_ORDER");
        expect(((dut.priority_pattern_id_flat_o >> (rank * 4)) & 0xf) ==
                   8 + expected_slots[rank], "GROUP_SLOT_PRIORITY_ORDER");
    }
    std::cout << "GROUP_SLOT_PRIORITY_ORDER " << (failures ? "FAIL" : "PASS") << '\n';

    if (failures) return 1;
    std::cout << "CANDIDATE_STORE_REGISTERED_BITS = 80\n";
    std::cout << "EXTRA_PERSISTENT_CONFIG_ID_BITS = 0\n";
    std::cout << "EXTRA_PATTERN_BITMAP_BITS = 0\n";
    std::cout << "EXTRA_DESCRIPTOR_BITS = 0\n";
    return 0;
}
