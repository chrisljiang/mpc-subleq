#ifndef SUBLEQ_SUBLEQ_PROGRAM_HPP__
#define SUBLEQ_SUBLEQ_PROGRAM_HPP__

#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "../libdpf/include/dpf/asio.hpp"
#include "../libdpf/include/dpf/bit.hpp"
#include "../libdpf/include/dpf/bitstring.hpp"
#include "../libdpf/include/dpf/eval_point.hpp"
#include "../libdpf/include/dpf/modint.hpp"
#include "../libdpf/include/dpf/prg.hpp"
#include "../libdpf/include/dpf/random.hpp"
#include "../libdpf/include/dpf/utils.hpp"
#include "../libdpf/include/dpf/wildcard.hpp"

#include "subleq_dot_prod.hpp"
#include "subleq_instruction.hpp"
#include "subleq_utils.hpp"

namespace subleq
{

template <std::size_t data_size, std::size_t data_per_code = 3, typename data_t = int32_t,
          typename node_t = std::array<data_t, data_per_code>, bool a_sub_b = false,
          typename rand_t = uint32_t, std::size_t div_bit = 0>
struct subleq_program
{
  public:
    static_assert(data_per_code == 3 || data_per_code == 4, "data per code must be 3 or 4");
    using subleq_instruction_t = subleq_instruction<data_per_code, data_t, node_t>;
    using subleq_dot_prod_t = subleq_dot_prod<data_size, data_per_code, data_t, node_t>;

    static constexpr std::size_t code_size = data_size - data_per_code + 1;
    static constexpr std::size_t code_bit_count = get_bit_count(code_size);
    static constexpr std::size_t code_items = get_item_count(code_size);

    static constexpr std::size_t data_bit_count = get_bit_count(data_size);
    static constexpr std::size_t data_items = get_item_count(data_size);

    static constexpr std::size_t div_num = std::size_t(1) << div_bit;

    using data_array_t = std::array<data_t, data_size>;
    using code_array_t = std::array<data_t, code_size>;

    using data_input_t = dpf::modint<data_bit_count>;
    using code_input_t = dpf::modint<code_bit_count>;

    using rand_dpf_t = dpf::utils::dpf_type_t<dpf::prg::aes128, dpf::prg::aes128, dpf::wildcard_value<rand_t>, dpf::bit>;

    data_array_t arr,            // database
                 bld;            // copy of other party's blinded database

    std::size_t cnt = 0;         // count of instructions processed
    data_t ret = 0;              // return value
    bool done = false,           // is program finished running
         party0 = false;         // is this subleq_program party0 in an MPC protocol
    rand_t seed = 0,             // seed for srand()
           oob_track = 0;        // tracker for if an oob memory access has occured

    subleq_program() = default;
    subleq_program(const subleq_program &) = delete;
    subleq_program(subleq_program &&) = default;
    subleq_program & operator=(const subleq_program &) = delete;
    subleq_program & operator=(subleq_program &&) = default;
    ~subleq_program() = default;

    subleq_program(const std::string file)
    {
        std::ifstream f;
        std::size_t temps;
        bool tempb;

        f.open(file);

        f >> temps;
        if (temps != data_size)
        {
            std::cerr << "DATA SIZE MISMATCH\n";
            exit(1);
        }

        f >> temps;
        if (temps != data_per_code)
        {
            std::cerr << "DATA PER CODE MISMATCH\n";
            exit(1);
        }

        f >> tempb;
        if (tempb != a_sub_b)
        {
            std::cerr << "A SUB B MISMATCH\n";
            exit(1);
        }

        f >> temps;
        if (temps != div_bit)
        {
            std::cerr << "DIV BIT MISMATCH\n";
            exit(1);
        }

        for (std::size_t i = 0; i < data_size; ++i)
        {
            if (f.eof() == true)
            {
                break;
            }
            f >> arr[i];
        }

        f.close();
    }

    void print_data(const std::size_t start = 0, const std::size_t end = data_size)
    {
        std::cout << "---- DATA ----\n";
        for (std::size_t i = start; i < end; ++i)
        {
            std::cout << i << ":\t" << arr[i] << '\n';
        }
        std::cout << '\n';
    }

    void print_shared_data(const subleq_program & prog0, const subleq_program & prog1,
        const std::size_t start = 0, const std::size_t end = data_size)
    {
        bool match = true;
        std::vector<std::size_t> wrong;
        std::cout << "---- SHARED DATA ----\n";
        for (std::size_t i = start; i < end; ++i)
        {
            data_t tmp = (prog0.arr[i] + prog1.arr[i]);
            if (arr[i] != tmp)
            {
                match = false;
                wrong.push_back(i);
            }
            std::cout << i << ":\t" << arr[i] << '\t' << tmp << '\n';
        }
        if (match == true)
        {
            std::cout << "Data matches!\n";
        }
        else
        {
            std::cout << "Data does not match!\n";
            std::cout << "Incorrect indices:\n\t";
            for (const std::size_t v : wrong)
            {
                std::cout << v << "\t";
            }
            std::cout << "\n";
        }
    }

    void run(std::size_t max_rounds = -1)
    {
        data_t cur_addr = 0, prev_addr;
        subleq_instruction_t cur_inst;
        while (max_rounds == -1 || cnt < max_rounds)
        {
            prev_addr = cur_addr;
            cur_inst.set(arr, cur_addr/div_num);  // get instruction
            if constexpr(a_sub_b == false)  // swap addresses if running in mem[b] -= mem[a] mode
            {
                data_t temp = cur_inst[0];
                cur_inst[0] = cur_inst[1];
                cur_inst[1] = temp;
            }

            if (static_cast<data_t>(cur_inst[0]/div_num) < 0 || static_cast<data_t>(cur_inst[0]/div_num) >= static_cast<data_t>(data_size)
                || static_cast<data_t>(cur_inst[1]/div_num) < 0 || static_cast<data_t>(cur_inst[1]/div_num) >= static_cast<data_t>(data_size))
            {
                std::cout << "Out of bounds access after round " << cnt << " due to data addresses being " << static_cast<data_t>(cur_inst[0]/div_num) << ", " << static_cast<data_t>(cur_inst[1]/div_num) << "\n";
                break;
            }

            arr[cur_inst[0]/div_num] = arr[cur_inst[0]/div_num] - arr[cur_inst[1]/div_num];  // do subtraction
            if (arr[cur_inst[0]/div_num] <= 0)
            {
                cur_addr = cur_inst[2];
            }
            else
            {
                if constexpr(data_per_code == 4)
                {
                    cur_addr = cur_inst[3];
                }
                else
                {
                    cur_addr += data_per_code*div_num;  // data_per_code must be 3 here
                }
            }

            ++cnt;

            if (cur_addr < 0)
            {
                done = true;
                break;
            }

            if (static_cast<data_t>(cur_addr/div_num) < -1 || static_cast<data_t>(cur_addr/div_num) >= static_cast<data_t>(code_size))
            {
                std::cout << "Out of bounds access after round " << cnt << " due to instruction address being " << static_cast<data_t>(cur_addr/div_num) << "\n";
                break;
            }
        }
        if (done == false && cnt == max_rounds)
        {
            std::cout << "Max number of rounds reached\n";
        }

        ret = arr[arr[prev_addr/div_num + data_t(a_sub_b == false)]/div_num];
    }

    auto additively_share()
    {
        auto ret = std::make_pair(subleq_program(), subleq_program());

        dpf::uniform_fill(ret.first.arr);

        // valid data indices are [0, data_size)
        // valid code indices are [-1, code_size)
        for (std::size_t i = 0; i < data_size; ++i)
        {
            ret.second.arr[i] = arr[i] - ret.first.arr[i];
        }

        ret.first.party0 = true;
        ret.second.party0 = false;

        ret.first.seed = dpf::uniform_sample<rand_t>();
        ret.second.seed = ret.first.seed;

        return std::move(ret);
    }

    template <typename PeerT>
    auto share_initial_blind(PeerT & peer, const subleq_dot_prod_t & dot_prod, ::asio::error_code & error)
    {
        for (std::size_t i = 0; i < data_size; ++i)
        {
            bld[i] = dot_prod.blind_d[i] + arr[i];
        }

        std::size_t bytes_written = ::asio::write(peer, ::asio::buffer(bld.data(), sizeof(data_array_t)), error);
        if (error)
        {
            return std::make_tuple(bytes_written, std::size_t(0));
        }

        std::size_t bytes_read = ::asio::read(peer, ::asio::buffer(bld.data(), sizeof(data_array_t)), error);

        return std::make_tuple(bytes_written, bytes_read);
    }

    template <typename PeerT>
    auto check_out_of_bounds(PeerT & peer, rand_dpf_t & dpf, ::asio::error_code & error)
    {
        rand_t share = oob_track * (party0 == true ? 1 : -1);
        bool my_bool;
        bool peer_bool;

        std::size_t bytes_written, bytes_read;
        std::tie(std::ignore, bytes_written, bytes_read) = dpf::asio::assign_wildcard_input(peer, dpf, share, error);
        if (error)
        {
            return std::make_tuple(true, bytes_written, bytes_read);
        }
        my_bool = *dpf::eval_point(dpf, rand_t(0));

        bytes_written += ::asio::write(peer, ::asio::buffer(&my_bool, sizeof(bool)), error);
        if (error)
        {
            return std::make_tuple(my_bool, bytes_written, bytes_read);
        }
        bytes_read += ::asio::read(peer, ::asio::buffer(&peer_bool, sizeof(bool)), error);

        return std::make_tuple(!(my_bool ^ peer_bool), bytes_written, bytes_read);
    }

    template <typename PeerT,
              typename data_iterable_t,
              typename code_iterable_t>
    auto read_code(PeerT & peer, const data_iterable_t & iter_a, const data_array_t & bld_a,
        const data_t w, const data_t bld_w, const code_iterable_t & iter_c, const bool local_end,
        const subleq_dot_prod_t & dot_prod_prev, const subleq_dot_prod_t & dot_prod_next, ::asio::error_code & error)
    {
        bool remote_end;
        code_array_t temp_c;
        subleq_instruction_t ret;
        data_t mult = party0 == true ? -1 : 1;

        auto it_a = std::begin(iter_a);
        auto it_c = std::begin(iter_c);

        for (std::size_t i = 0; i < data_size; ++i, ++it_a)
        {
            arr[i] += mult * (*it_a * (w + bld_w) + dot_prod_prev.blind_w * bld_a[i] + dot_prod_prev.cancel_w[i]);
            bld[i] = dot_prod_next.blind_d[i] + arr[i];
        }

        for (std::size_t i = 0; i < code_size; ++i, ++it_c)
        {
            temp_c[i] = dot_prod_next.blind_c[i] + *it_c;
        }

        std::size_t bytes_written = ::asio::write(peer,
            std::array<::asio::const_buffer, 3> {
                ::asio::buffer(&local_end, sizeof(bool)),
                ::asio::buffer(temp_c.data(), sizeof(code_array_t)),
                ::asio::buffer(bld.data(), sizeof(data_array_t))
            }, error);
        if (error)
        {
            return std::make_tuple(ret, bytes_written, std::size_t(0));
        }
        std::size_t bytes_read = ::asio::read(peer,
            std::array<::asio::mutable_buffer, 3> {
                ::asio::buffer(&remote_end, sizeof(bool)),
                ::asio::buffer(temp_c.data(), sizeof(code_array_t)),
                ::asio::buffer(bld.data(), sizeof(data_array_t))
            }, error);
        if (error)
        {
            return std::make_tuple(ret, bytes_written, bytes_read);
        }

        if (static_cast<bool>(local_end ^ remote_end) == true)
        {
            done = true;
        }
        else
        {
            it_c = std::begin(iter_c);

            for (std::size_t i = 0; i < code_size; ++i, ++it_c)
            {
                for (std::size_t j = 0; j < data_per_code; ++j)
                {
                    ret[j] += *it_c * (arr[i+j] + bld[i+j]) + dot_prod_next.blind_d[i+j] * temp_c[i];
                }
            }
            ret = (ret + dot_prod_next.cancel_c) * mult;
        }

        return std::make_tuple(ret, bytes_written, bytes_read);
    }

    template <typename PeerT,
              typename data_iterable_t,
              typename code_iterable_t,
              typename DpfKey>
    auto read_code_with_oob_check(PeerT & peer, const data_iterable_t & iter_a, const data_array_t & bld_a,
        const data_t w, const data_t bld_w, const code_iterable_t & iter_c, const bool local_end, DpfKey & dpf_oob,
        const subleq_dot_prod_t & dot_prod_prev, const subleq_dot_prod_t & dot_prod_next, ::asio::error_code & error)
    {
        bool remote_end;
        code_array_t temp_c;
        subleq_instruction_t ret;
        data_t mult = party0 == true ? -1 : 1;

        auto it_a = std::begin(iter_a);
        auto it_c = std::begin(iter_c);

        for (std::size_t i = 0; i < data_size; ++i, ++it_a)
        {
            arr[i] += mult * (*it_a * (w + bld_w) + dot_prod_prev.blind_w * bld_a[i] + dot_prod_prev.cancel_w[i]);
            bld[i] = dot_prod_next.blind_d[i] + arr[i];
        }

        for (std::size_t i = 0; i < code_size; ++i, ++it_c)
        {
            temp_c[i] = dot_prod_next.blind_c[i] + *it_c;
        }

        rand_t offset_share = dpf_oob.offset_x.compute_and_get_share(oob_track * (party0 == true ? 1 : -1));

        std::size_t bytes_written = ::asio::write(peer,
            std::array<::asio::const_buffer, 4> {
                ::asio::buffer(&local_end, sizeof(bool)),
                ::asio::buffer(temp_c.data(), sizeof(code_array_t)),
                ::asio::buffer(bld.data(), sizeof(data_array_t)),
                ::asio::buffer(&offset_share, sizeof(rand_t))
            }, error);
        if (error)
        {
            return std::make_tuple(ret, offset_share, bytes_written, std::size_t(0));
        }
        std::size_t bytes_read = ::asio::read(peer,
            std::array<::asio::mutable_buffer, 4> {
                ::asio::buffer(&remote_end, sizeof(bool)),
                ::asio::buffer(temp_c.data(), sizeof(code_array_t)),
                ::asio::buffer(bld.data(), sizeof(data_array_t)),
                ::asio::buffer(&offset_share, sizeof(rand_t))
            }, error);
        if (error)
        {
            return std::make_tuple(ret, offset_share, bytes_written, bytes_read);
        }

        if (static_cast<bool>(local_end ^ remote_end) == true)
        {
            done = true;
        }
        else
        {
            it_c = std::begin(iter_c);

            for (std::size_t i = 0; i < code_size; ++i, ++it_c)
            {
                for (std::size_t j = 0; j < data_per_code; ++j)
                {
                    ret[j] += *it_c * (arr[i+j] + bld[i+j]) + dot_prod_next.blind_d[i+j] * temp_c[i];
                }
            }
            ret = (ret + dot_prod_next.cancel_c) * mult;

            offset_share = dpf_oob.offset_x.reconstruct(offset_share);
        }

        return std::make_tuple(ret, offset_share, bytes_written, bytes_read);
    }

    template <typename PeerT,
              typename data_iterable_t>
    auto read_data(PeerT & peer, const data_iterable_t & iter_a, data_array_t & bld_a,
        const data_iterable_t & iter_b, const subleq_dot_prod_t & dot_prod,
        ::asio::error_code & error)
    {
        data_array_t temp_b;
        data_t ret_a = 0;
        data_t ret_b = 0;

        std::array<::asio::mutable_buffer, 2> buffers {
            ::asio::buffer(bld_a.data(), sizeof(data_array_t)),
            ::asio::buffer(temp_b.data(), sizeof(data_array_t))
        };

        auto it_a = std::begin(iter_a);
        auto it_b = std::begin(iter_b);

        for (std::size_t i = 0; i < data_size; ++i, ++it_a, ++it_b)
        {
            bld_a[i] = dot_prod.blind_a[i] + *it_a;
            temp_b[i] = dot_prod.blind_b[i] + *it_b;
        }

        std::size_t bytes_written = ::asio::write(peer, buffers, error);
        if (error)
        {
            return std::make_tuple(ret_a, ret_b, bytes_written, std::size_t(0));
        }
        std::size_t bytes_read = ::asio::read(peer, buffers, error);
        if (error)
        {
            return std::make_tuple(ret_a, ret_b, bytes_written, bytes_read);
        }

        it_a = std::begin(iter_a);
        it_b = std::begin(iter_b);

        for (std::size_t i = 0; i < data_size; ++i, ++it_a, ++it_b)
        {
            ret_a += *it_a * (arr[i] + bld[i]) + dot_prod.blind_d[i] * bld_a[i];
            ret_b += *it_b * (arr[i] + bld[i]) + dot_prod.blind_d[i] * temp_b[i];
        }
        ret_a += dot_prod.cancel_a;
        ret_b += dot_prod.cancel_b;

        if (party0 == true)
        {
            ret_a *= -1;
            ret_b *= -1;
        }

        return std::make_tuple(ret_a, ret_b, bytes_written, bytes_read);
    }

    template <typename PeerT>
    auto write_subleq_program(PeerT & peer, ::asio::error_code & error)
    {
        std::size_t bytes_written = ::asio::write(peer,
            std::array<::asio::const_buffer, 8> {
                ::asio::buffer(&arr, sizeof(data_array_t)),
                ::asio::buffer(&bld, sizeof(data_array_t)),
                ::asio::buffer(&cnt, sizeof(std::size_t)),
                ::asio::buffer(&ret, sizeof(data_t)),
                ::asio::buffer(&done, sizeof(bool)),
                ::asio::buffer(&party0, sizeof(bool)),
                ::asio::buffer(&seed, sizeof(rand_t)),
                ::asio::buffer(&oob_track, sizeof(rand_t))
            }, error);

        return bytes_written;
    }

    template <typename PeerT>
    auto read_subleq_program(PeerT & peer, ::asio::error_code & error)
    {
        std::size_t bytes_read = ::asio::read(peer,
            std::array<::asio::mutable_buffer, 8> {
                ::asio::buffer(&arr, sizeof(data_array_t)),
                ::asio::buffer(&bld, sizeof(data_array_t)),
                ::asio::buffer(&cnt, sizeof(std::size_t)),
                ::asio::buffer(&ret, sizeof(data_t)),
                ::asio::buffer(&done, sizeof(bool)),
                ::asio::buffer(&party0, sizeof(bool)),
                ::asio::buffer(&seed, sizeof(rand_t)),
                ::asio::buffer(&oob_track, sizeof(rand_t))
            }, error);

        return bytes_read;
    }

};

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_PROGRAM_HPP__
