#include "Vrecam_dss_g2x2_r_static_selector.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>

namespace {

struct Path {
    unsigned a;
    unsigned b;
    unsigned c;
    unsigned d;
};

bool legal(const Path& path) {
    const bool a_row_release = (path.a & 1U) != 0U;
    const bool c_row_borrow = (path.c & 2U) != 0U;
    const bool b_col_release = (path.b & 1U) != 0U;
    const bool a_col_borrow = (path.a & 2U) != 0U;
    const bool d_row_release = (path.d & 1U) != 0U;
    const bool b_row_borrow = (path.b & 2U) != 0U;
    const bool c_col_release = (path.c & 1U) != 0U;
    const bool d_col_borrow = (path.d & 2U) != 0U;

    return (!c_row_borrow || a_row_release) &&
           (!a_col_borrow || b_col_release) &&
           (!b_row_borrow || d_row_release) &&
           (!d_col_borrow || c_col_release);
}

std::array<Path, 81> static_paths() {
    std::array<Path, 81> paths{};
    unsigned index = 0U;
    for (unsigned d = 0U; d < 4U; ++d) {
        for (unsigned c = 0U; c < 4U; ++c) {
            for (unsigned b = 0U; b < 4U; ++b) {
                for (unsigned a = 0U; a < 4U; ++a) {
                    const Path path{a, b, c, d};
                    if (legal(path)) {
                        paths.at(index++) = path;
                    }
                }
            }
        }
    }
    if (index != paths.size()) {
        std::cerr << "unexpected static path count\n";
        std::exit(1);
    }
    return paths;
}

unsigned dense_for_slot(unsigned slot) { return slot == 3U ? 0U : slot; }

bool map_has_slot(unsigned map, unsigned sa, unsigned slot) {
    return (map & (1U << (sa * 3U + dense_for_slot(slot)))) != 0U;
}

bool path_valid(unsigned map, const Path& path) {
    return map_has_slot(map, 0U, path.a) &&
           map_has_slot(map, 1U, path.b) &&
           map_has_slot(map, 2U, path.c) &&
           map_has_slot(map, 3U, path.d);
}

std::array<std::uint32_t, 2> store_image(unsigned map) {
    std::array<std::uint32_t, 2> image{};
    for (unsigned sa = 0U; sa < 4U; ++sa) for (unsigned dense = 0U; dense < 3U; ++dense) {
        const unsigned offset = 5U * (sa * 3U + dense);
        const unsigned record = ((1U + sa * 3U + dense) << 1U) |
            ((map & (1U << (sa * 3U + dense))) ? 1U : 0U);
        for (unsigned bit = 0U; bit < 5U; ++bit) if (record & (1U << bit))
            image[(offset + bit) / 32U] |= 1U << ((offset + bit) % 32U);
    }
    return image;
}

unsigned config_for_slot(unsigned, unsigned slot) { return slot == 1U ? 4U : (slot == 2U ? 2U : 0U); }

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

unsigned map_for_paths(const std::array<Path, 81>& paths,
                       std::initializer_list<unsigned> path_ids) {
    unsigned map = 0U;
    for (const unsigned id : path_ids) {
        const Path& path = paths.at(id);
        map |= 1U << dense_for_slot(path.a);
        map |= 1U << (3U + dense_for_slot(path.b));
        map |= 1U << (6U + dense_for_slot(path.c));
        map |= 1U << (9U + dense_for_slot(path.d));
    }
    return map;
}

void drive_store_image(Vrecam_dss_g2x2_r_static_selector& dut, unsigned map) {
    const auto image = store_image(map);
    dut.candidate_store_image_i = static_cast<std::uint64_t>(image[0]) |
        (static_cast<std::uint64_t>(image[1]) << 32U);
        dut.eval();
}

void check_directed_priority(Vrecam_dss_g2x2_r_static_selector& dut,
                             const std::array<Path, 81>& paths,
                             unsigned map, unsigned expected_path) {
    drive_store_image(dut, map);
    const Path& expected = paths.at(expected_path);
    require(dut.selected_valid_o != 0U, "directed priority path not selected");
    require(dut.selected_a_slot_o == expected.a &&
            dut.selected_b_slot_o == expected.b &&
            dut.selected_c_slot_o == expected.c &&
            dut.selected_d_slot_o == expected.d,
            "directed priority selected the wrong static path");
}

}  // namespace

int main() {
    const auto paths = static_paths();
    unsigned priority_selection_mismatches = 0U;
    unsigned selected_path_legality_errors = 0U;
    unsigned config_decode_errors = 0U;
    unsigned pattern_read_errors = 0U;

    Vrecam_dss_g2x2_r_static_selector dut;
    check_directed_priority(dut, paths, map_for_paths(paths, {0U}), 0U);
    drive_store_image(dut, 0U);
    require(dut.selected_valid_o == 0U, "empty map selected a path");

    for (unsigned map = 0U; map < (1U << 12U); ++map) {
        drive_store_image(dut, map);

        unsigned selected_index = paths.size();
        for (unsigned index = 0U; index < paths.size(); ++index) {
            if (path_valid(map, paths[index])) {
                selected_index = index;
                break;
            }
        }

        if (selected_index == paths.size()) {
            if (dut.selected_valid_o != 0U) {
                ++priority_selection_mismatches;
            }
            continue;
        }

        const Path expected = paths[selected_index];
        if (dut.selected_valid_o == 0U ||
            dut.selected_a_slot_o != expected.a ||
            dut.selected_b_slot_o != expected.b ||
            dut.selected_c_slot_o != expected.c ||
            dut.selected_d_slot_o != expected.d) {
            ++priority_selection_mismatches;
            continue;
        }
        if (!legal(expected)) {
            ++selected_path_legality_errors;
        }
        if (dut.selected_a_config_id_o != config_for_slot(0U, expected.a) ||
            dut.selected_b_config_id_o != config_for_slot(1U, expected.b) ||
            dut.selected_c_config_id_o != config_for_slot(2U, expected.c) ||
            dut.selected_d_config_id_o != config_for_slot(3U, expected.d)) {
            ++config_decode_errors;
        }
        if (dut.selected_a_pattern_id_o != ((1U + dense_for_slot(expected.a)) & 0xfU) ||
            dut.selected_b_pattern_id_o != ((4U + dense_for_slot(expected.b)) & 0xfU) ||
            dut.selected_c_pattern_id_o != ((7U + dense_for_slot(expected.c)) & 0xfU) ||
            dut.selected_d_pattern_id_o != ((10U + dense_for_slot(expected.d)) & 0xfU)) {
            ++pattern_read_errors;
        }
    }

    std::cout << "RTL_DENSE_VALIDITY_MAPS_CHECKED: 4096\n";
    std::cout << "PRIORITY_DIRECTED_CASES: 1\n";
    std::cout << "PRIORITY_SELECTION_MISMATCHES: " << priority_selection_mismatches << '\n';
    std::cout << "SELECTED_PATH_LEGALITY_ERRORS: " << selected_path_legality_errors << '\n';
    std::cout << "CONFIG_DECODE_ERRORS: " << config_decode_errors << '\n';
    std::cout << "PATTERN_READ_ERRORS: " << pattern_read_errors << '\n';
    require(priority_selection_mismatches == 0U, "fixed priority mismatch");
    require(selected_path_legality_errors == 0U, "selected illegal path");
    require(config_decode_errors == 0U, "config decode mismatch");
    require(pattern_read_errors == 0U, "pattern read mismatch");
    return 0;
}
