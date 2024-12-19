#include <array>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <ios>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include "asio.hpp"
#define LIBDPF_HAS_ASIO
#include "dpf.hpp"
#include "grotto.hpp"
#include "subleq.hpp"

#include "constants.hpp"

#ifndef PARTY
#define PARTY "0"
#endif

int main()
{
    constexpr bool p0 = (PARTY == "0");

    // Timing data
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::nanoseconds pre_pro, mpc_run;

    // Communication data
    std::size_t bytes_written_peer,
                bytes_read_peer,
                bytes_read_p2,
                bytes_just_written,
                bytes_just_read,
                rounds_of_communication;

    // Setup connections with other parties
    asio::io_context io_context;
    asio::error_code error{};
    asio::ip::tcp::socket peer(io_context),
                          p2(io_context);
    asio::ip::tcp::no_delay nagle_toggle{true};
    asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_QUICKACK> quickack_toggle{true};

    if constexpr(p0 == true)
    {
        std::cout << "Connecting to p2 on port "
                  << std::to_string(p0_p2) << std::endl;
        asio::ip::tcp::resolver resolver2(io_context);
        while (true)
        {
            try
            {
                asio::connect(p2, resolver2.resolve("localhost", std::to_string(p0_p2)));
                break;
            }
            catch (std::system_error) { }
        }

        std::cout << "Connecting to p1 on port "
                  << std::to_string(p0_p1) << std::endl;
        asio::ip::tcp::resolver resolver1(io_context);
        while (true)
        {
            try
            {
                asio::connect(peer, resolver1.resolve("localhost", std::to_string(p0_p1)));
                break;
            }
            catch (std::system_error) { }
        }
    }
    else
    {
        std::cout << "Connecting to p2 on port "
                  << std::to_string(p1_p2) << std::endl;
        asio::ip::tcp::resolver resolver2(io_context);
        while (true)
        {
            try
            {
                asio::connect(p2, resolver2.resolve("localhost", std::to_string(p1_p2)));
                break;
            }
            catch (std::system_error) { }
        }

        std::cout << "Waiting for p0 to connect on port "
                  << std::to_string(p0_p1) << std::endl;
        asio::ip::tcp::acceptor acceptor0(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), p0_p1));
        asio::ip::tcp::endpoint ep0;
        acceptor0.accept(peer, ep0);
    }

    peer.set_option(nagle_toggle);
    peer.set_option(quickack_toggle);
    p2.set_option(nagle_toggle);
    p2.set_option(quickack_toggle);

    // Setup database dpfs
    constexpr subleq_div<p0, div_bit, data_t> div;
    using data_input_t = dpf::modint<data_bit_count>;
    using code_input_t = dpf::modint<code_bit_count>;
    using data_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<data_input_t>, data_t>;
    using code_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<code_input_t>, data_t>;

    std::vector<data_dpf_t> adpf;
    adpf.reserve(num_rounds);
    std::vector<data_dpf_t> bdpf;
    bdpf.reserve(num_rounds);
    std::vector<code_dpf_t> idpf;
    idpf.reserve(num_rounds);

    std::vector<std::vector<data_t>> aread(num_rounds, std::vector<data_t>(data_items));
    std::vector<std::vector<data_t>> bread(num_rounds, std::vector<data_t>(data_items));
    std::vector<std::vector<data_t>> iread(num_rounds, std::vector<data_t>(code_items));

    dpf::basic_interval_memoizer<data_dpf_t> amemo = dpf::make_basic_full_memoizer<data_dpf_t>();
    dpf::basic_interval_memoizer<data_dpf_t> bmemo = dpf::make_basic_full_memoizer<data_dpf_t>();
    dpf::basic_interval_memoizer<code_dpf_t> imemo = dpf::make_basic_full_memoizer<code_dpf_t>();

    std::vector<dpf::wildcard_input_iterable<const data_dpf_t, std::vector<data_t>::iterator>> aiter;
    aiter.reserve(num_rounds);
    std::vector<dpf::wildcard_input_iterable<const data_dpf_t, std::vector<data_t>::iterator>> biter;
    biter.reserve(num_rounds);
    std::vector<dpf::wildcard_input_iterable<const code_dpf_t, std::vector<data_t>::iterator>> iiter;
    iiter.reserve(num_rounds);

    // Setup database dot product
    using dot_prod_t = subleq::subleq_dot_prod<data_size, data_per_code, data_t, node_t>;

    std::vector<dot_prod_t> dot;
    dot.reserve(num_rounds);

    // Setup prefix parity
    using prefix_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<data_t>, dpf::bit>;

    // Gives buckets [-MAX, 0] and [1, MAX]
    const std::array<data_t, 2> endpoints {std::numeric_limits<data_t>::min(), 1};
    // Gives buckets [0, data_size*div_num) and complement
    const std::array<data_t, 2> oob_d_endpoints {0, static_cast<data_t>(data_size*div_num)};
    // Gives buckets [-div_num, code_size*div_num) and complement
    const std::array<data_t, 2> oob_c_endpoints {static_cast<data_t>(-div_num), static_cast<data_t>(code_size*div_num)};

    std::vector<prefix_dpf_t> pdpf;
    pdpf.reserve(num_rounds);
    std::vector<prefix_dpf_t> oob_d_a_dpf;
    oob_d_a_dpf.reserve(num_rounds);
    std::vector<prefix_dpf_t> oob_d_b_dpf;
    oob_d_b_dpf.reserve(num_rounds);
    std::vector<prefix_dpf_t> oob_c_dpf;
    oob_c_dpf.reserve(num_rounds);

    // Setup next address getter
    using next_address_t = subleq::subleq_next_address<data_t>;

    std::vector<next_address_t> next;
    next.reserve(num_rounds);

    // Setup oob checking
    using rand_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<rand_t>, dpf::bit>;

    std::vector<rand_dpf_t> rdpf;
    rdpf.reserve(num_oob_checks);

    // Setup long living variables
    subleq_program_t prog;
    subleq_instruction_t curins;
    data_t curins_addr(0);
    data_t aval, bld_aval, bval;
    bool oob_occurred = false;

    using data_array_t = std::array<data_t, data_size>;
    data_array_t bld_a;

    constexpr data_input_t data_input_min{0};
    constexpr data_input_t data_input_max{data_size-1};
    constexpr code_input_t code_input_min{0};
    constexpr code_input_t code_input_max{code_size-1};

    // Preprocessing
    start = std::chrono::system_clock::now();
    std::cout << "Reading from p2\n";

    // Get additively shared subleq program
    std::cout << "Receiving additively shared subleq program" << std::endl;
    bytes_read_p2 = prog.read_subleq_program(p2, error);
    if (error) throw error;
    std::srand(prog.seed);

    // Receive 2 num_rounds sets of prefix parity DPFs and data read DPFs
    std::cout << "Receiving data read dpfs" << std::endl;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, oob_d_a_dpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<data_dpf_t>(p2, adpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, oob_d_b_dpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<data_dpf_t>(p2, bdpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    // Receive num_rounds sets of prefix parity DPFs and code read DPFs
    std::cout << "Receiving code read dpfs" << std::endl;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, oob_c_dpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<code_dpf_t>(p2, idpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    // Receive num_rounds prefix parity DPFs for less than or equal to zero check
    std::cout << "Receiving prefix dpfs" << std::endl;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, pdpf, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    // Receive Du-Atallah dot product blinds and cancellation terms for reading and writing
    std::cout << "Receiving dot-product blinds" << std::endl;
    std::tie(bytes_just_read, std::ignore) = dot_prod_t::read_dot_prod(p2, dot, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    // Receive aby2 style scaler product
    std::cout << "Receiving aby2 scalar product info" << std::endl;
    std::tie(bytes_just_read, std::ignore) = next_address_t::read_next_address(p2, next, num_rounds, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    // Receive num_rounds oob checking DPF of size number of bits of rand_t
    std::cout << "Receiving oob dpfs" << std::endl;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<rand_dpf_t>(p2, rdpf, num_oob_checks, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    // Perform evalfulls
    std::cout << "Performing evalfulls\n";
    for (std::size_t i = 0; i < num_rounds; ++i)
    {
        aiter.emplace_back(dpf::eval_interval(adpf[i], data_input_min, data_input_max, aread[i], amemo, dpf::wildcard_input_tag));
        biter.emplace_back(dpf::eval_interval(bdpf[i], data_input_min, data_input_max, bread[i], bmemo, dpf::wildcard_input_tag));
        iiter.emplace_back(dpf::eval_interval(idpf[i], code_input_min, code_input_max, iread[i], imemo, dpf::wildcard_input_tag));
    }

    end = std::chrono::system_clock::now();
    pre_pro = end - start;

    // Run subleq
    start = std::chrono::system_clock::now();
    std::cout << "Running subleq\n";

    // Get first instruction
    curins.set(prog.arr, 0);

    // If running in mem[B] -= mem[A] (as opposed to mem[A] -= mem[B]), switch addresses
    if constexpr(a_sub_b == false)
    {
        data_t temp = curins[0];
        curins[0] = curins[1];
        curins[1] = temp;
    }

    // Share initial blind of program and peform shifts
    std::tie(std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::share_initial_blind_and_assign_wildcard_input_2_with_oob(peer, prog, div, dot[0], adpf[prog.cnt], oob_d_a_dpf[prog.cnt], curins[0], bdpf[prog.cnt], oob_d_b_dpf[prog.cnt], curins[1], error);
    bytes_written_peer = bytes_just_written;
    bytes_read_peer = bytes_just_read;
    rounds_of_communication = 1;
    if (error) throw error;

    while (true)
    {
        if (prog.cnt % print_rounds == 0)
        {
            std::cout << "Round: " << prog.cnt << std::endl;
        }

        // Update out of bounds tracker
        prog.oob_track ^= grotto::segment_parities(oob_d_a_dpf[prog.cnt], oob_d_endpoints)[1] * std::rand()
            ^ grotto::segment_parities(oob_d_b_dpf[prog.cnt], oob_d_endpoints)[1] * std::rand();

        // Perform PIR lookup against prog.data
        std::tie(aval, bval, bytes_just_written, bytes_just_read) = prog.read_data(peer, aiter[prog.cnt].get(), bld_a, biter[prog.cnt].get(), dot[prog.cnt], error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;

        // Get result of subtraction return value
        prog.ret = aval - bval;
        aval = -bval;

        // Perform shifts and prefix parity calculation
        std::tie(std::ignore, bytes_just_written, bytes_just_read) = dpf::asio::assign_wildcard_input(peer, pdpf[prog.cnt], prog.ret, error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;
        bool segment = grotto::segment_parities(pdpf[prog.cnt], endpoints)[0];

        // Get next instruction address
        if constexpr(data_per_code == 3)
        {
            curins[3] = static_cast<data_t>(curins_addr + data_per_code * (prog.party0 == true) * div_num);
        }
        std::tie(curins_addr, bytes_just_written, bytes_just_read) = next[prog.cnt].evaluate(peer, static_cast<data_t>(segment) * (prog.party0 == true ? 1 : -1), curins[2], curins[3], error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;

        // Perform shift to get instruction to read, and update data to write
        std::tie(std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::assign_wildcard_input_1_with_oob(peer, div, dot[prog.cnt], idpf[prog.cnt], oob_c_dpf[prog.cnt], curins_addr, aval, bld_aval, error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;

        // Update out of bounds tracker
        prog.oob_track ^= grotto::segment_parities(oob_c_dpf[prog.cnt], oob_c_endpoints)[1] * std::rand();

        // Write data, perform PIR lookup against prog.code, check if current instruction is last instruction, and shift out-of-bounds DPF if required
        if constexpr(oob_check != 0)
        {
            if ((prog.cnt+1) % oob_check == 0)
            {
                std::tie(curins, std::ignore, bytes_just_written, bytes_just_read) = prog.read_code_with_oob_check(peer, aiter[prog.cnt].get(), bld_a, aval, bld_aval, iiter[prog.cnt].get(), dpf::eval_point(oob_c_dpf[prog.cnt], -1), rdpf[prog.cnt / oob_check], dot[prog.cnt], dot[prog.cnt+1], error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
            }
            else
            {
                std::tie(curins, bytes_just_written, bytes_just_read) = prog.read_code(peer, aiter[prog.cnt].get(), bld_a, aval, bld_aval, iiter[prog.cnt].get(), dpf::eval_point(oob_c_dpf[prog.cnt], -1), dot[prog.cnt], dot[prog.cnt+1], error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
            }
        }
        else
        {
            std::tie(curins, bytes_just_written, bytes_just_read) = prog.read_code(peer, aiter[prog.cnt].get(), bld_a, aval, bld_aval, iiter[prog.cnt].get(), dpf::eval_point(oob_c_dpf[prog.cnt], -1), dot[prog.cnt], dot[prog.cnt+1], error);
            bytes_written_peer += bytes_just_written;
            bytes_read_peer += bytes_just_read;
            ++rounds_of_communication;
            if (error) throw error;
        }

        ++prog.cnt;

        // Check if program is done
        if (prog.done == true) {
            std::cerr << "GOOD EXIT FROM P" << PARTY << " AFTER " << prog.cnt << " ROUNDS\n";
            break;
        }

        // Check if insufficient rounds exist
        if (prog.cnt >= num_rounds - 1) {
            std::cerr << "INSUFFICIENT ROUNDS FROM P" << PARTY << "\n";
            break;
        }

        // If running in mem[B] -= mem[A] (as opposed to mem[A] -= mem[B]), switch addresses
        if constexpr(a_sub_b == false)
        {
            data_t temp = curins[0];
            curins[0] = curins[1];
            curins[1] = temp;
        }

        // Check if out of bounds access has occurred and perform shifts
        if constexpr(oob_check != 0)
        {
            if (prog.cnt % oob_check == 0)
            {
                std::tie(oob_occurred, std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::check_out_of_bounds_and_assign_wildcard_input_2_with_oob(peer, div, dpf::eval_point(rdpf[(prog.cnt-1) / oob_check], 0), adpf[prog.cnt], oob_d_a_dpf[prog.cnt], curins[0], bdpf[prog.cnt], oob_d_b_dpf[prog.cnt], curins[1], error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
                if (oob_occurred == true)
                {
                    std::cerr << "OUT OF BOUNDS ACCESS FROM P" << PARTY << " AFTER " << prog.cnt << " ROUNDS\n";
                    break;
                }
            }
            else
            {
                std::tie(std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::assign_wildcard_input_2_with_oob(peer, div, adpf[prog.cnt], oob_d_a_dpf[prog.cnt], curins[0], bdpf[prog.cnt], oob_d_b_dpf[prog.cnt], curins[1], error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
            }
        }
        else
        {
            std::tie(std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::assign_wildcard_input_2_with_oob(peer, div, adpf[prog.cnt], oob_d_a_dpf[prog.cnt], curins[0], bdpf[prog.cnt], oob_d_b_dpf[prog.cnt], curins[1], error);
            bytes_written_peer += bytes_just_written;
            bytes_read_peer += bytes_just_read;
            ++rounds_of_communication;
            if (error) throw error;
        }
    }

    // If program did not terminate due to out of bounds access, check if out of bounds access has occurred
    if (oob_occurred == false)
    {
        std::tie(oob_occurred, bytes_just_written, bytes_just_read) = prog.check_out_of_bounds(peer, rdpf[num_oob_checks-1], error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        rounds_of_communication += 2;
        if (error) throw error;
        if (oob_occurred == true)
        {
            std::cerr << "OUT OF BOUNDS ACCESS FROM P" << PARTY << " AFTER " << prog.cnt << " ROUNDS\n";
        }
    }

    end = std::chrono::system_clock::now();
    mpc_run = end - start;

    // Send subleq program back to parent for reconstruction and verification
    prog.write_subleq_program(p2, error);
    if (error) throw error;

    std::cout << std::dec
              << "Preprocessing (nano): " << pre_pro.count() << '\n'
              << "MPC run (nano): " << mpc_run.count() << '\n'
              << "Preprocessing communication from P2 (bytes): " << bytes_read_p2 << '\n'
              << "Online written to peer (bytes): " << bytes_written_peer << '\n'
              << "Online read from peer (bytes): " << bytes_read_peer << '\n'
              << "Online rounds of communication: " << rounds_of_communication << '\n';
}
