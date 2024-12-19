#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <ios>
#include <iostream>
#include <limits>
#include <list>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>

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
                bytes_written_p2 = 0,
                bytes_read_p2,
                bytes_just_written,
                bytes_just_read,
                rounds_of_communication;

    // Setup connections with other parties
    asio::io_context io_context;
    asio::error_code error{};
    asio::ip::tcp::socket peer(io_context),
                          p2(io_context), p2_receiver(io_context);
    asio::ip::tcp::no_delay nagle_toggle{true};
    asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_QUICKACK> quickack_toggle{true};

    if constexpr(p0 == true)
    {
        std::cout << "Connecting to p2 on ports "
                  << std::to_string(p0_p2_1) << " and "
                  << std::to_string(p0_p2_2) << std::endl;
        asio::ip::tcp::resolver resolver2(io_context),
                                resolver2_receiver(io_context);
        while(true)
        {
            try
            {
                asio::connect(p2, resolver2.resolve("localhost", std::to_string(p0_p2_1)));
                break;
            }
            catch (std::system_error) { }
        }
        while(true)
        {
            try
            {
                asio::connect(p2_receiver, resolver2_receiver.resolve("localhost", std::to_string(p0_p2_2)));
                break;
            }
            catch (std::system_error) { }
        }

        std::cout << "Connecting to p1 on port "
                  << std::to_string(p0_p1) << std::endl;
        asio::ip::tcp::resolver resolver1(io_context);
        while(true)
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
        std::cout << "Connecting to p2 on ports "
                  << std::to_string(p1_p2_1) << " and "
                  << std::to_string(p1_p2_2) << std::endl;
        asio::ip::tcp::resolver resolver2(io_context),
                                resolver2_receiver(io_context);
        while(true)
        {
            try
            {
                asio::connect(p2, resolver2.resolve("localhost", std::to_string(p1_p2_1)));
                break;
            }
            catch (std::system_error) { }
        }
        while(true)
        {
            try
            {
                asio::connect(p2_receiver, resolver2_receiver.resolve("localhost", std::to_string(p1_p2_2)));
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
    p2_receiver.set_option(nagle_toggle);
    p2_receiver.set_option(quickack_toggle);

    // Setup database dpfs
    constexpr subleq_div<p0, div_bit, data_t> div;
    using data_input_t = dpf::modint<data_bit_count>;
    using code_input_t = dpf::modint<code_bit_count>;
    using data_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<data_input_t>, data_t>;
    using code_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<code_input_t>, data_t>;

    std::list<data_dpf_t> adpf;
    std::list<data_dpf_t> bdpf;
    std::list<code_dpf_t> idpf;

    auto adpf_iter = std::begin(adpf);
    auto bdpf_iter = std::begin(bdpf);
    auto idpf_iter = std::begin(idpf);

    std::list<std::array<data_t, data_items>> aread;
    std::list<std::array<data_t, data_items>> bread;
    std::list<std::array<data_t, code_items>> iread;

    dpf::basic_interval_memoizer<data_dpf_t> amemo = dpf::make_basic_full_memoizer<data_dpf_t>();
    dpf::basic_interval_memoizer<data_dpf_t> bmemo = dpf::make_basic_full_memoizer<data_dpf_t>();
    dpf::basic_interval_memoizer<code_dpf_t> imemo = dpf::make_basic_full_memoizer<code_dpf_t>();

    std::list<dpf::wildcard_input_iterable<const data_dpf_t, std::array<data_t, data_items>::iterator>> aiter;
    std::list<dpf::wildcard_input_iterable<const data_dpf_t, std::array<data_t, data_items>::iterator>> biter;
    std::list<dpf::wildcard_input_iterable<const code_dpf_t, std::array<data_t, code_items>::iterator>> iiter;

    // Setup database dot product
    using dot_prod_t = subleq::subleq_dot_prod<data_size, data_per_code, data_t, node_t>;

    std::list<dot_prod_t> dot;

    // Setup prefix parity
    using prefix_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<data_t>, dpf::bit>;

    // Gives buckets [-MAX, 0] and [1, MAX]
    const std::array<data_t, 2> endpoints {std::numeric_limits<data_t>::min(), 1};
    // Gives buckets [0, data_size*div_num) and complement
    const std::array<data_t, 2> oob_d_endpoints {0, static_cast<data_t>(data_size*div_num)};
    // Gives buckets [-div_num, code_size*div_num) and complement
    const std::array<data_t, 2> oob_c_endpoints {static_cast<data_t>(-div_num), static_cast<data_t>(code_size*div_num)};

    std::list<prefix_dpf_t> pdpf;
    std::list<prefix_dpf_t> oob_d_a_dpf;
    std::list<prefix_dpf_t> oob_d_b_dpf;
    std::list<prefix_dpf_t> oob_c_dpf;

    // Setup next address getter
    using next_address_t = subleq::subleq_next_address<data_t>;

    std::list<next_address_t> next;

    // Setup oob checking
    using rand_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<rand_t>, dpf::bit>;

    std::list<rand_dpf_t> rdpf;
    std::list<rand_dpf_t> rdpf_fin;

    // Setup long living variables
    subleq_program_t prog;
    subleq_instruction_t curins;
    data_t curins_addr(0);
    data_t aval, bld_aval, bval;
    bool oob_occurred = false;
    dot_prod_t dot_prev;

    using data_array_t = std::array<data_t, data_size>;
    data_array_t bld_a;

    constexpr data_input_t data_input_min{0};
    constexpr data_input_t data_input_max{data_size-1};
    constexpr code_input_t code_input_min{0};
    constexpr code_input_t code_input_max{code_size-1};

    // Prepare threads
    std::atomic_bool finished(false);
    std::atomic_size_t available(0);

    // Preprocessing
    start = std::chrono::system_clock::now();
    std::cout << "Reading from p2\n";

    // Get additively shared subleq program
    std::cout << "Receiving additively shared subleq program" << std::endl;
    bytes_read_p2 = prog.read_subleq_program(p2, error);
    if (error) throw error;
    std::srand(prog.seed);

    // Get one oob checking DPF of size number of bits of rand_t to be used as final check
    std::cout << "Receiving one oob dpf for final check" << std::endl;
    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<rand_dpf_t>(p2, rdpf_fin, 1, error);
    bytes_read_p2 += bytes_just_read;
    if (error) throw error;

    end = std::chrono::system_clock::now();
    pre_pro = end - start;

    // Run subleq
    start = std::chrono::system_clock::now();
    std::cout << "Running subleq\n";

    // Receive data in thread
    std::thread receiver([&p2=p2_receiver, error, &finished, &available, &adpf, &adpf_iter, &aiter, &bdpf, &bdpf_iter, &biter, &pdpf, &oob_d_a_dpf, &oob_d_b_dpf, &oob_c_dpf, &idpf, &idpf_iter, &iiter, &dot, &next, &amemo, &aread, &bmemo, &bread, &imemo, &iread, &rdpf, &bytes_written_p2, &bytes_read_p2, bytes_just_written, bytes_just_read, &data_input_min, &data_input_max, &code_input_min, &code_input_max]() mutable
    {
        std::size_t cnt = 0;

        // Get dpfs, dot product blinds, and next address getters from p2
        while (finished.load() == false && cnt <= max_rounds)
        {
            if (cnt % print_rounds == 0)
            {
                std::cout << "Received: " << (cnt * buf_rounds) << std::endl;
            }

            // Wait if too many rounds are buffered already
            while (available.load() > buf_rounds && finished.load() == false)
            {
                // std::this_thread::yield();
                // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            // Receive 2 buf_rounds sets of prefix parity DPFs and data read DPFs
            std::cout << "Receiving data read dpfs" << std::endl;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, oob_d_a_dpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<data_dpf_t>(p2, adpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, oob_d_b_dpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<data_dpf_t>(p2, bdpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;

            // Receive buf_rounds sets of prefix parity DPFs and code read DPFs
            std::cout << "Receiving code read dpfs" << std::endl;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, oob_c_dpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<code_dpf_t>(p2, idpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;

            // Receive buf_rounds prefix parity DPFs for less than or equal to zero check
            std::cout << "Receiving prefix dpfs" << std::endl;
            std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<prefix_dpf_t>(p2, pdpf, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;

            // Receive Du-Atallah dot product blinds and cancellation terms for reading and writing
            std::cout << "Receiving dot-product blinds" << std::endl;
            std::tie(bytes_just_read, std::ignore) = dot_prod_t::read_dot_prod(p2, dot, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;

            // Receive aby2 style scaler product
            std::cout << "Receiving aby2 scalar product info" << std::endl;
            std::tie(bytes_just_read, std::ignore) = next_address_t::read_next_address(p2, next, buf_rounds, error);
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;

            // Receive buf_rounds oob checking DPF of size number of bits of rand_t
            if constexpr(oob_check != 0)
            {
                if (cnt % oob_check == 0)
                {
                    std::cout << "Receiving oob dpfs" << std::endl;
                    std::tie(bytes_just_read, std::ignore) = dpf::asio::read_dpf<rand_dpf_t>(p2, rdpf, buf_rounds, error);
                    bytes_read_p2 += bytes_just_read;
                    if (error) throw error;
                }
            }

            // Perform evalfulls
            std::cout << "Performing evalfulls\n";
            for (std::size_t i = 0; i < buf_rounds; ++i)
            {
                if (cnt == 0 && i == 0)
                {
                    adpf_iter = std::begin(adpf);
                    bdpf_iter = std::begin(bdpf);
                    idpf_iter = std::begin(idpf);
                }
                else
                {
                    ++adpf_iter;
                    ++bdpf_iter;
                    ++idpf_iter;
                }

                aread.emplace_back();
                aiter.emplace_back(dpf::eval_interval(*adpf_iter, data_input_min, data_input_max, aread.back(), amemo, dpf::wildcard_input_tag));
                bread.emplace_back();
                biter.emplace_back(dpf::eval_interval(*bdpf_iter, data_input_min, data_input_max, bread.back(), bmemo, dpf::wildcard_input_tag));
                iread.emplace_back();
                iiter.emplace_back(dpf::eval_interval(*idpf_iter, code_input_min, code_input_max, iread.back(), imemo, dpf::wildcard_input_tag));
            }

            // Sync with p2
            std::cout << "Syncing with p2" << std::endl;
            std::tie(bytes_just_written, bytes_just_read) = subleq::asio::sync_peer(p2, error);
            bytes_written_p2 += bytes_just_written;
            bytes_read_p2 += bytes_just_read;
            if (error) throw error;

            // Update counter
            available += buf_rounds;

            ++cnt;
        }
    });

    // Get first instruction
    curins.set(prog.arr, 0);

    // Wait for first round of dpf's and other data to be ready
    while (available.load() < 1)
    {
        // std::this_thread::yield();
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // If running in mem[B] -= mem[A] (as opposed to mem[A] -= mem[B]), switch addresses
    if constexpr(a_sub_b == false)
    {
        data_t temp = curins[0];
        curins[0] = curins[1];
        curins[1] = temp;
    }

    // Share initial blind of program and peform shifts
    std::tie(std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::share_initial_blind_and_assign_wildcard_input_2_with_oob(peer, prog, div, dot.front(), adpf.front(), oob_d_a_dpf.front(), curins[0], bdpf.front(), oob_d_b_dpf.front(), curins[1], error);
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

        // Mark round of dpf's and other data as used
        --available;

        // Update out of bounds tracker
        prog.oob_track ^= grotto::segment_parities(oob_d_a_dpf.front(), oob_d_endpoints)[1] * std::rand()
            ^ grotto::segment_parities(oob_d_b_dpf.front(), oob_d_endpoints)[1] * std::rand();

        // Perform PIR lookup against prog.data
        std::tie(aval, bval, bytes_just_written, bytes_just_read) = prog.read_data(peer, aiter.front().get(), bld_a, biter.front().get(), dot.front(), error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;

        // Get result of subtraction return value
        prog.ret = aval - bval;
        aval = -bval;

        // Perform shifts and prefix parity calculation
        std::tie(std::ignore, bytes_just_written, bytes_just_read) = dpf::asio::assign_wildcard_input(peer, pdpf.front(), prog.ret, error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;
        bool segment = grotto::segment_parities(pdpf.front(), endpoints)[0];

        // Get next instruction address
        if constexpr(data_per_code == 3)
        {
            curins[3] = static_cast<data_t>(curins_addr + data_per_code * (prog.party0 == true) * div_num);
        }
        std::tie(curins_addr, bytes_just_written, bytes_just_read) = next.front().evaluate(peer, static_cast<data_t>(segment) * (prog.party0 == true ? 1 : -1), curins[2], curins[3], error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;

        // Perform shift to get instruction to read, and update data to write
        std::tie(std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::assign_wildcard_input_1_with_oob(peer, div, dot.front(), idpf.front(), oob_c_dpf.front(), curins_addr, aval, bld_aval, error);
        bytes_written_peer += bytes_just_written;
        bytes_read_peer += bytes_just_read;
        ++rounds_of_communication;
        if (error) throw error;

        // Update out of bounds tracker
        prog.oob_track ^= grotto::segment_parities(oob_c_dpf.front(), oob_c_endpoints)[1] * std::rand();

        // Wait for next round of dpf's and other data to be ready
        while (available.load() < 1)
        {
            // std::this_thread::yield();
            // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        dot_prev = std::move(dot.front());
        dot.pop_front();

        // Write data, perform PIR lookup against prog.code, check if current instruction is last instruction, and shift out-of-bounds DPF if required
        if constexpr(oob_check != 0)
        {
            if ((prog.cnt+1) % oob_check == 0)
            {
                std::tie(curins, std::ignore, bytes_just_written, bytes_just_read) = prog.read_code_with_oob_check(peer, aiter.front().get(), bld_a, aval, bld_aval, iiter.front().get(), dpf::eval_point(oob_c_dpf.front(), -1), rdpf.front(), dot_prev, dot.front(), error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
            }
            else
            {
                std::tie(curins, bytes_just_written, bytes_just_read) = prog.read_code(peer, aiter.front().get(), bld_a, aval, bld_aval, iiter.front().get(), dpf::eval_point(oob_c_dpf.front(), -1), dot_prev, dot.front(), error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
            }
        }
        else
        {
            std::tie(curins, bytes_just_written, bytes_just_read) = prog.read_code(peer, aiter.front().get(), bld_a, aval, bld_aval, iiter.front().get(), dpf::eval_point(oob_c_dpf.front(), -1), dot_prev, dot.front(), error);
            bytes_written_peer += bytes_just_written;
            bytes_read_peer += bytes_just_read;
            ++rounds_of_communication;
            if (error) throw error;
        }

        ++prog.cnt;

        adpf.pop_front();
        bdpf.pop_front();
        oob_d_a_dpf.pop_front();
        oob_d_b_dpf.pop_front();

        // Check if program is done
        if (prog.done == true) {
            std::cerr << "GOOD EXIT FROM P" << PARTY << " AFTER " << prog.cnt << " ROUNDS\n";
            break;
        }

        // Check if maximum rounds reached
        if (prog.cnt >= max_rounds) {
            std::cerr << "MAXIMUM ROUNDS REACHED FROM P" << PARTY << "\n";
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
                std::tie(oob_occurred, std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::check_out_of_bounds_and_assign_wildcard_input_2_with_oob(peer, div, dpf::eval_point(rdpf.front(), 0), adpf.front(), oob_d_a_dpf.front(), curins[0], bdpf.front(), oob_d_b_dpf.front(), curins[1], error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
                rdpf.pop_front();
                if (oob_occurred == true)
                {
                    std::cerr << "OUT OF BOUNDS ACCESS FROM P" << PARTY << " AFTER " << prog.cnt << " ROUNDS\n";
                    break;
                }
            }
            else
            {
                std::tie(std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::assign_wildcard_input_2_with_oob(peer, div, adpf.front(), oob_d_a_dpf.front(), curins[0], bdpf.front(), oob_d_b_dpf.front(), curins[1], error);
                bytes_written_peer += bytes_just_written;
                bytes_read_peer += bytes_just_read;
                ++rounds_of_communication;
                if (error) throw error;
            }
        }
        else
        {
            std::tie(std::ignore, std::ignore, bytes_just_written, bytes_just_read) = subleq::asio::assign_wildcard_input_2_with_oob(peer, div, adpf.front(), oob_d_a_dpf.front(), curins[0], bdpf.front(), oob_d_b_dpf.front(), curins[1], error);
            bytes_written_peer += bytes_just_written;
            bytes_read_peer += bytes_just_read;
            ++rounds_of_communication;
            if (error) throw error;
        }

        // Pop rest of used dpf's and other data
        pdpf.pop_front();
        oob_c_dpf.pop_front();
        idpf.pop_front();
        next.pop_front();
        aread.pop_front();
        bread.pop_front();
        iread.pop_front();
        aiter.pop_front();
        biter.pop_front();
        iiter.pop_front();
    }

    // If program did not terminate due to out of bounds access, check if out of bounds access has occurred
    if (oob_occurred == false)
    {
        std::tie(oob_occurred, bytes_just_written, bytes_just_read) = prog.check_out_of_bounds(peer, rdpf_fin.front(), error);
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

    finished.store(true);
    receiver.join();

    std::cout << std::dec
              << "Preprocessing (nano): " << pre_pro.count() << '\n'
              << "MPC run (nano): " << mpc_run.count() << '\n'
              << "Communication to P2 (bytes): " << bytes_written_p2 << '\n'
              << "Communication from P2 (bytes): " << bytes_read_p2 << '\n'
              << "Online written to peer (bytes): " << bytes_written_peer << '\n'
              << "Online read from peer (bytes): " << bytes_read_peer << '\n'
              << "Online rounds of communication: " << rounds_of_communication << '\n';
}
