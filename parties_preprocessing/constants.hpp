#ifndef CONSTANTS_HPP__
#define CONSTANTS_HPP__

#include <array>
#include <string>

#include "subleq.hpp"

// Editable variables
// Note: num_rounds must be at least 1 greater than number of rounds from a non-mpc based run
//       since the exit condition is checked in the middle of a round
constexpr std::size_t num_rounds = 695;
constexpr std::size_t data_size = 179;
constexpr std::size_t data_per_code = 3;
constexpr bool a_sub_b = false;  // true if mem[a] -= mem[b] instead of mem[b] -= mem[a]
constexpr std::size_t div_bit = 0;
std::string file_name = "../sample_code/mult_constant_time_uniform_3.sq";
constexpr std::size_t oob_check = 1;  // 0 => never check until program end, otherwise check every oob_check rounds
constexpr std::size_t print_rounds = 100;  // number of rounds between printing round info

// Ports
constexpr int p0_p1 = 64322;
constexpr int p0_p2 = 64320;
constexpr int p1_p2 = 64321;

// Setup subleq
using data_t = int32_t;
using node_t = std::array<data_t, data_per_code>;
// using node_t = simde__m128i;  // for use when data_t = int32_t and data_per_code = 4
using rand_t = uint32_t;
using subleq_instruction_t = subleq::subleq_instruction<data_per_code, data_t, node_t>;
using subleq_program_t = subleq::subleq_program<data_size, data_per_code, data_t, node_t, a_sub_b, rand_t, div_bit>;

constexpr std::size_t code_size = data_size - data_per_code + 1;
constexpr std::size_t code_bit_count = subleq::get_bit_count(code_size);
constexpr std::size_t code_items = subleq::get_item_count(code_size);

constexpr std::size_t data_bit_count = subleq::get_bit_count(data_size);
constexpr std::size_t data_items = subleq::get_item_count(data_size);

constexpr std::size_t div_num = std::size_t(1) << div_bit;

constexpr std::size_t num_oob_checks = (oob_check == 0 ? 1 : num_rounds / oob_check + 1);

#endif // CONSTANTS_HPP__
