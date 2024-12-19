#include <array>
#include <chrono>
#include <ios>
#include <iostream>
#include <string>
#include <tuple>

#include "asio.hpp"
#define LIBDPF_HAS_ASIO
#include "dpf.hpp"
#include "grotto.hpp"
#include "subleq.hpp"

#include "constants.hpp"

int main()
{
    // Timing data
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::nanoseconds reg_run, pre_pro, mpc_run;

    // Communication data
    std::size_t bytes_written_0,
                bytes_written_1,
                bytes_just_written_0,
                bytes_just_written_1;

    // Read plaintext subleq code and data
    subleq_program_t prog_norm = subleq_program_t(file_name);
    std::array<subleq_program_t, 2> prog;
    std::tie(prog[0], prog[1]) = prog_norm.additively_share();

    // Run non-mpc subleq
    start = std::chrono::system_clock::now();
    prog_norm.run(num_rounds);
    end = std::chrono::system_clock::now();
    reg_run = end - start;

    // Setup connections with other parties
    asio::io_context io_context;
    asio::error_code error{};
    asio::ip::tcp::socket p0(io_context),
                          p1(io_context);
    asio::ip::tcp::no_delay nagle_toggle{true};
    asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_QUICKACK> quickack_toggle{true};

    std::cout << "Waiting for p1 to connect on port "
              << std::to_string(p1_p2) << std::endl;
    asio::ip::tcp::acceptor acceptor1(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), p1_p2));
    asio::ip::tcp::endpoint ep1;
    acceptor1.accept(p1, ep1);

    std::cout << "Waiting for p0 to connect on port "
              << std::to_string(p0_p2) << std::endl;
    asio::ip::tcp::acceptor acceptor0(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), p0_p2));
    asio::ip::tcp::endpoint ep0;
    acceptor0.accept(p0, ep0);

    p0.set_option(nagle_toggle);
    p0.set_option(quickack_toggle);
    p1.set_option(nagle_toggle);
    p1.set_option(quickack_toggle);

    // Setup database dpfs
    using data_input_t = dpf::modint<data_bit_count>;
    using code_input_t = dpf::modint<code_bit_count>;
    auto data_dpf_t = dpf::make_dpfargs(dpf::wildcard<data_input_t>, data_t(1));
    auto code_dpf_t = dpf::make_dpfargs(dpf::wildcard<code_input_t>, data_t(1));

    // Setup database dot product
    using dot_prod_t = subleq::subleq_dot_prod<data_size, data_per_code, data_t, node_t>;

    // Setup prefix parity
    auto prefix_dpf_t = dpf::make_dpfargs(dpf::wildcard<data_t>, dpf::bit(1));

    // Setup next address getter
    using next_address_t = subleq::subleq_next_address<data_t>;

    // Setup oob checking
    auto rand_dpf_t = dpf::make_dpfargs(dpf::wildcard<rand_t>, dpf::bit(1));

    // Setup for generating DPFs with same random distinguished point
    std::vector<std::tuple<data_t, data_t, data_t>> xs;

    // Preprocessing
    start = std::chrono::system_clock::now();
    std::cout << "Writing to p0 and p1\n";

    // Send additively shared subleq program
    std::cout << "Sending additively shared subleq program" << std::endl;
    bytes_written_0 = prog[0].write_subleq_program(p0, error);
    if (error) throw error;
    bytes_written_1 = prog[1].write_subleq_program(p1, error);
    if (error) throw error;

    // Generate and send 2 num_rounds sets of prefix parity DPFs and data read DPFs
    std::cout << "Sending data read dpfs" << std::endl;
    std::tie(xs, bytes_just_written_0, bytes_just_written_1, std::ignore) = subleq::asio::make_dpf_ret_input(p0, p1, num_rounds, prefix_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = subleq::asio::make_dpf_set_input(p0, p1, num_rounds, xs, data_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;
    std::tie(xs, bytes_just_written_0, bytes_just_written_1, std::ignore) = subleq::asio::make_dpf_ret_input(p0, p1, num_rounds, prefix_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = subleq::asio::make_dpf_set_input(p0, p1, num_rounds, xs, data_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;

    // Generate and send num_rounds sets of prefix parity DPFs and code read DPFs
    std::cout << "Sending code read dpfs" << std::endl;
    std::tie(xs, bytes_just_written_0, bytes_just_written_1, std::ignore) = subleq::asio::make_dpf_ret_input(p0, p1, num_rounds, prefix_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = subleq::asio::make_dpf_set_input(p0, p1, num_rounds, xs, code_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;

    // Generate and send num_rounds prefix parity DPFs for less than or equal to zero check
    std::cout << "Sending prefix dpfs" << std::endl;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = dpf::asio::make_dpf(p0, p1, num_rounds, prefix_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;

    // Generate and send Du-Atallah dot product blinds and cancellation terms for reading and writing
    std::cout << "Sending dot-product blinds" << std::endl;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = dot_prod_t::make_dot_prod(p0, p1, num_rounds, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;

    // Generate and send aby2 style scaler product
    std::cout << "Sending aby2 scalar product info" << std::endl;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = next_address_t::make_next_address(p0, p1, num_rounds, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;

    // Generate and send num_oob_checks oob checking DPF of size number of bits of rand_t
    std::cout << "Sending oob dpfs" << std::endl;
    std::tie(bytes_just_written_0, bytes_just_written_1, std::ignore) = dpf::asio::make_dpf(p0, p1, num_oob_checks, rand_dpf_t, error);
    bytes_written_0 += bytes_just_written_0;
    bytes_written_1 += bytes_just_written_1;
    if (error) throw error;

    end = std::chrono::system_clock::now();
    pre_pro = end - start;

    // Run subleq
    start = std::chrono::system_clock::now();

    // Read shares of subleq program for reconstruction and verification
    std::cout << "Waiting for subleq to finish\n";
    prog[0].read_subleq_program(p0, error);
    if (error) throw error;
    prog[1].read_subleq_program(p1, error);
    if (error) throw error;

    end = std::chrono::system_clock::now();
    mpc_run = end - start;

    // Print subleq results and timing data
    prog_norm.print_shared_data(prog[0], prog[1]);
    std::cout << std::dec
              << "Number of rounds: " << prog_norm.cnt << '\n'
              << "Correct subleq output: " << prog_norm.ret << '\n'
              << "MPC subleq output: " << (prog[0].ret + prog[1].ret) << '\n'
              << "Regular run (nano): " << reg_run.count() << '\n'
              << "Preprocessing (nano): " << pre_pro.count() << '\n'
              << "MPC run (nano): " << mpc_run.count() << '\n'
              << "Preprocessing communication to P0 (bytes): " << bytes_written_0 << '\n'
              << "Preprocessing communication to P1 (bytes): " << bytes_written_1 << '\n';
}
