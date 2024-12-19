#ifndef SUBLEQ_SUBLEQ_DOT_PROD_HPP__
#define SUBLEQ_SUBLEQ_DOT_PROD_HPP__

#include <array>
#include <tuple>
#include <utility>

#include "../libdpf/include/dpf/bitstring.hpp"
#include "../libdpf/include/dpf/random.hpp"

#include "subleq_instruction.hpp"

namespace subleq
{

template <std::size_t data_size, std::size_t data_per_code = 4,
          typename data_t = int32_t, typename node_t = std::array<data_t, data_per_code>>
struct subleq_dot_prod
{
  public:
    static constexpr std::size_t code_size = data_size - data_per_code + 1;

    using data_array_t = std::array<data_t, data_size>;
    using code_array_t = std::array<data_t, code_size>;
    using subleq_instruction_t = subleq_instruction<data_per_code, data_t, node_t>;

    subleq_dot_prod(data_array_t a, data_array_t b, code_array_t c, data_array_t d, data_t w,
        data_t can_a, data_t can_b, subleq_instruction_t can_c, data_array_t can_w)
      : blind_a(a), blind_b(b), blind_c(c), blind_d(d), blind_w(w),
        cancel_a(can_a), cancel_b(can_b), cancel_c(can_c), cancel_w(can_w) { }
    subleq_dot_prod() = default;
    subleq_dot_prod(const subleq_dot_prod &) = delete;
    subleq_dot_prod(subleq_dot_prod &&) = default;
    subleq_dot_prod & operator=(const subleq_dot_prod &) = delete;
    subleq_dot_prod & operator=(subleq_dot_prod &&) = default;

    data_array_t blind_a;  // blind data read A
    data_array_t blind_b;  // blind data read B
    code_array_t blind_c;  // blind intruction read
    data_array_t blind_d;  // blind data base
    data_t blind_w;  // blind data for writing A
    data_t cancel_a;
    data_t cancel_b;
    subleq_instruction_t cancel_c;
    data_array_t cancel_w;

    static auto make_subleq_dot_prod()
    {
        data_array_t a0, a1, b0, b1, d0, d1;
        code_array_t c0, c1;
        data_t w0, w1;
        data_t tmp_a, tmp_b, can_a0 = 0, can_a1 = 0, can_b0 = 0, can_b1 = 0;
        subleq_instruction_t tmp_c, can_c0, can_c1;
        data_array_t tmp_w, can_w0, can_w1;

        dpf::uniform_fill(a0);
        dpf::uniform_fill(a1);
        dpf::uniform_fill(b0);
        dpf::uniform_fill(b1);
        dpf::uniform_fill(c0);
        dpf::uniform_fill(c1);
        dpf::uniform_fill(d0);
        dpf::uniform_fill(d1);
        dpf::uniform_fill(w0);
        dpf::uniform_fill(w1);
        dpf::uniform_fill(tmp_a);
        dpf::uniform_fill(tmp_b);
        dpf::uniform_fill(tmp_c);
        dpf::uniform_fill(tmp_w);

        for (std::size_t i = 0; i < data_size; ++i)
        {
            can_a0 += a0[i] * d1[i];
            can_a1 += a1[i] * d0[i];
            can_b0 += b0[i] * d1[i];
            can_b1 += b1[i] * d0[i];
            can_w0[i] = a0[i] * w1 + tmp_w[i];
            can_w1[i] = a1[i] * w0 + tmp_w[i];
        }
        can_a0 += tmp_a;
        can_a1 += tmp_a;
        can_b0 += tmp_b;
        can_b1 += tmp_b;

        for (std::size_t i = 0; i < code_size; ++i)
        {
            for (std::size_t j = 0; j < data_per_code; ++j)
            {
                can_c0[j] += c0[i] * d1[i + j];
                can_c1[j] += c1[i] * d0[i + j];
            }
        }
        can_c0 = can_c0 + tmp_c;
        can_c1 = can_c1 + tmp_c;

        return std::make_pair
        (
            subleq_dot_prod(std::move(a0), std::move(b0), std::move(c0), std::move(d0), std::move(w0),
                can_a0, can_b0, std::move(can_c0), std::move(can_w0)),
            subleq_dot_prod(std::move(a1), std::move(b1), std::move(c1), std::move(d1), std::move(w1),
                can_a1, can_b1, std::move(can_c1), std::move(can_w1))
        );
    }

    template <typename PeerT>
    static auto make_dot_prod(PeerT & peer0, PeerT & peer1, std::size_t count, ::asio::error_code & error)
    {
        std::size_t bytes_written0 = 0,
                    bytes_written1 = 0;
        for (std::size_t num_written = 0; num_written < count; ++num_written)
        {
            auto [dot0, dot1] = make_subleq_dot_prod();

            bytes_written0 += ::asio::write(peer0,
                std::array<::asio::const_buffer, 9> {
                    ::asio::buffer(&dot0.blind_a, sizeof(data_array_t)),
                    ::asio::buffer(&dot0.blind_b, sizeof(data_array_t)),
                    ::asio::buffer(&dot0.blind_c, sizeof(code_array_t)),
                    ::asio::buffer(&dot0.blind_d, sizeof(data_array_t)),
                    ::asio::buffer(&dot0.blind_w, sizeof(data_t)),
                    ::asio::buffer(&dot0.cancel_a, sizeof(data_t)),
                    ::asio::buffer(&dot0.cancel_b, sizeof(data_t)),
                    ::asio::buffer(&dot0.cancel_c, sizeof(subleq_instruction_t)),
                    ::asio::buffer(&dot0.cancel_w, sizeof(data_array_t))
                }, error);
            if (error)
            {
                return std::make_tuple(bytes_written0, bytes_written1, num_written);
            }

            bytes_written1 += ::asio::write(peer1,
                std::array<::asio::const_buffer, 9> {
                    ::asio::buffer(&dot1.blind_a, sizeof(data_array_t)),
                    ::asio::buffer(&dot1.blind_b, sizeof(data_array_t)),
                    ::asio::buffer(&dot1.blind_c, sizeof(code_array_t)),
                    ::asio::buffer(&dot1.blind_d, sizeof(data_array_t)),
                    ::asio::buffer(&dot1.blind_w, sizeof(data_t)),
                    ::asio::buffer(&dot1.cancel_a, sizeof(data_t)),
                    ::asio::buffer(&dot1.cancel_b, sizeof(data_t)),
                    ::asio::buffer(&dot1.cancel_c, sizeof(subleq_instruction_t)),
                    ::asio::buffer(&dot1.cancel_w, sizeof(data_array_t))
                }, error);
            if (error)
            {
                return std::make_tuple(bytes_written0, bytes_written1, num_written);
            }
        }

        return std::make_tuple(bytes_written0, bytes_written1, count);
    }

    template <typename PeerT,
              typename BackEmplaceable>
    static auto read_dot_prod(PeerT & peer, BackEmplaceable & output, std::size_t count, ::asio::error_code & error)
    {
        data_array_t a;
        data_array_t b;
        code_array_t c;
        data_array_t d;
        data_t w;
        data_t can_a;
        data_t can_b;
        subleq_instruction_t can_c;
        data_array_t can_w;

        std::size_t bytes_read = 0;
        for (std::size_t num_read = 0; num_read < count; ++num_read)
        {
            bytes_read += ::asio::read(peer,
                std::array<::asio::mutable_buffer, 9> {
                    ::asio::buffer(&a, sizeof(data_array_t)),
                    ::asio::buffer(&b, sizeof(data_array_t)),
                    ::asio::buffer(&c, sizeof(code_array_t)),
                    ::asio::buffer(&d, sizeof(data_array_t)),
                    ::asio::buffer(&w, sizeof(data_t)),
                    ::asio::buffer(&can_a, sizeof(data_t)),
                    ::asio::buffer(&can_b, sizeof(data_t)),
                    ::asio::buffer(&can_c, sizeof(subleq_instruction_t)),
                    ::asio::buffer(&can_w, sizeof(data_array_t))
                }, error);
            if (error)
            {
                return std::make_tuple(bytes_read, num_read);
            }

            output.emplace_back(a, b, c, d, w, can_a, can_b, can_c, can_w);
        }

        return std::make_tuple(bytes_read, count);
    }

};

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_DOT_PROD_HPP__
