#ifndef SUBLEQ_SUBLEQ_ASIO_HPP__
#define SUBLEQ_SUBLEQ_ASIO_HPP__

#include <array>
#include <tuple>
#include <vector>

#include "asio.hpp"
#define LIBDPF_HAS_ASIO
#include "../libdpf/include/dpf/asio.hpp"
#include "../libdpf/include/dpf/eval_point.hpp"

#include "subleq_div.hpp"
#include "subleq_dot_prod.hpp"
#include "subleq_dpf.hpp"
#include "subleq_next_address.hpp"
#include "subleq_program.hpp"

namespace subleq
{

namespace asio
{

template <typename PeerT>
auto sync_peer(PeerT & peer, ::asio::error_code & error)
{
    bool synced = false;

    std::size_t bytes_written = ::asio::write(peer, ::asio::buffer(&synced, sizeof(bool)), error);
    if (error)
    {
        return std::make_tuple(bytes_written, std::size_t(0));
    }

    std::size_t bytes_read = ::asio::read(peer, ::asio::buffer(&synced, sizeof(bool)), error);

    return std::make_tuple(bytes_written, bytes_read);
}

template <typename PeerT,
          typename DataT,
          bool Party,
          std::size_t DivBit,
          typename DpfKey0,
          typename DpfKey1>
auto assign_wildcard_input_2_with_oob(PeerT & peer,
    const subleq_div<Party, DivBit, DataT> & div,
    DpfKey0 & dpf_a, DpfKey1 & dpf_a_oob, DataT input_share_a,
    DpfKey0 & dpf_b, DpfKey1 & dpf_b_oob, DataT input_share_b,
    ::asio::error_code & error)
{
    using input_type_0 = typename DpfKey0::input_type;
    using input_type_1 = typename DpfKey1::input_type;

    input_type_1 offset_share_a = dpf_a_oob.offset_x.compute_and_get_share(div(input_share_a));
    input_type_1 offset_share_b = dpf_b_oob.offset_x.compute_and_get_share(div(input_share_b));

    std::array<::asio::mutable_buffer, 2> offset_shares {
        ::asio::buffer(&offset_share_a, sizeof(input_type_1)),
        ::asio::buffer(&offset_share_b, sizeof(input_type_1))
    };

    std::size_t bytes_written = ::asio::write(peer, offset_shares, error);
    if (error)
    {
        return std::make_tuple(offset_share_a, offset_share_b, bytes_written, std::size_t(0));
    }
    std::size_t bytes_read = ::asio::read(peer, offset_shares, error);
    if (!error)
    {
        offset_share_a = dpf_a_oob.offset_x.reconstruct(offset_share_a);
        offset_share_b = dpf_b_oob.offset_x.reconstruct(offset_share_b);
        dpf_a.offset_x.set(static_cast<input_type_0>(offset_share_a));
        dpf_b.offset_x.set(static_cast<input_type_0>(offset_share_b));
    }

    return std::make_tuple(offset_share_a, offset_share_b, bytes_written, bytes_read);
}

template <std::size_t I0 = 1,
          std::size_t I1 = 2,
          std::size_t I2 = 3,
          typename PeerT,
          typename DpfKey,
          typename OutputType>
auto assign_wildcard_output_3(PeerT & peer, DpfKey & dpf, OutputType && output_share_0, OutputType && output_share_1, OutputType && output_share_2, ::asio::error_code & error)
{
    using dpf_type = DpfKey;
    using leaf_type_0 = std::tuple_element_t<I0, typename dpf_type::leaf_tuple>;
    using leaf_type_1 = std::tuple_element_t<I1, typename dpf_type::leaf_tuple>;
    using leaf_type_2 = std::tuple_element_t<I2, typename dpf_type::leaf_tuple>;
    using output_type_0 = typename dpf_type::concrete_output_type<I0>;
    using output_type_1 = typename dpf_type::concrete_output_type<I1>;
    using output_type_2 = typename dpf_type::concrete_output_type<I2>;

    auto & leaf_wrapper_0 = std::get<I0>(dpf.leaf_nodes);
    auto & leaf_wrapper_1 = std::get<I1>(dpf.leaf_nodes);
    auto & leaf_wrapper_2 = std::get<I2>(dpf.leaf_nodes);

    auto blinded_output_0 = leaf_wrapper_0.compute_and_get_blinded_output_share(output_share_0);
    auto blinded_output_1 = leaf_wrapper_1.compute_and_get_blinded_output_share(output_share_1);
    auto blinded_output_2 = leaf_wrapper_2.compute_and_get_blinded_output_share(output_share_2);

    std::array<::asio::mutable_buffer, 3> blinded_outputs {
        ::asio::buffer(&blinded_output_0, sizeof(output_type_0)),
        ::asio::buffer(&blinded_output_1, sizeof(output_type_1)),
        ::asio::buffer(&blinded_output_2, sizeof(output_type_2))
    };

    std::size_t bytes_written = ::asio::write(peer, blinded_outputs, error);
    if (error)
    {
        return std::make_tuple(bytes_written, std::size_t(0));
    }
    std::size_t bytes_read = ::asio::read(peer, blinded_outputs, error);
    if (error)
    {
        return std::make_tuple(bytes_written, bytes_read);
    }

    leaf_type_0 leaf_share_0 = leaf_wrapper_0.compute_and_get_leaf_share(blinded_output_0);
    leaf_type_1 leaf_share_1 = leaf_wrapper_1.compute_and_get_leaf_share(blinded_output_1);
    leaf_type_2 leaf_share_2 = leaf_wrapper_2.compute_and_get_leaf_share(blinded_output_2);

    std::array<::asio::mutable_buffer, 3> leaf_shares {
        ::asio::buffer(&leaf_share_0, sizeof(leaf_type_0)),
        ::asio::buffer(&leaf_share_1, sizeof(leaf_type_1)),
        ::asio::buffer(&leaf_share_2, sizeof(leaf_type_2))
    };

    bytes_written += ::asio::write(peer, leaf_shares, error);
    if (error)
    {
        return std::make_tuple(bytes_written, bytes_read);
    }
    bytes_read += ::asio::read(peer, leaf_shares, error);
    if (!error)
    {
        leaf_share_0 = leaf_wrapper_0.reconstruct_correction_word(leaf_share_0);
        leaf_share_1 = leaf_wrapper_1.reconstruct_correction_word(leaf_share_1);
        leaf_share_2 = leaf_wrapper_2.reconstruct_correction_word(leaf_share_2);
    }

    return std::make_tuple(bytes_written, bytes_read);
}

template <typename PeerT,
          typename DataT,
          typename NodeT,
          typename RandT,
          bool ASubB,
          bool Party,
          std::size_t DivBit,
          std::size_t DataSize,
          std::size_t DataPerCode,
          typename DpfKey0,
          typename DpfKey1>
auto share_initial_blind_and_assign_wildcard_input_2_with_oob(PeerT & peer,
    subleq_program<DataSize, DataPerCode, DataT, NodeT, ASubB, RandT, DivBit> & prog,
    const subleq_div<Party, DivBit, DataT> & div,
    const subleq_dot_prod<DataSize, DataPerCode, DataT, NodeT> & dot_prod,
    DpfKey0 & dpf_a, DpfKey1 & dpf_a_oob, DataT input_share_a,
    DpfKey0 & dpf_b, DpfKey1 & dpf_b_oob, DataT input_share_b,
    ::asio::error_code & error)
{
    using subleq_program_t = subleq_program<DataSize, DataPerCode, DataT, NodeT, ASubB, RandT, DivBit>;
    using data_array_t = typename subleq_program_t::data_array_t;
    constexpr std::size_t data_size = DataSize;

    using input_type_0 = typename DpfKey0::input_type;
    using input_type_1 = typename DpfKey1::input_type;

    for (std::size_t i = 0; i < data_size; ++i)
    {
        prog.bld[i] = dot_prod.blind_d[i] + prog.arr[i];
    }

    input_type_1 offset_share_a = dpf_a_oob.offset_x.compute_and_get_share(div(input_share_a));
    input_type_1 offset_share_b = dpf_b_oob.offset_x.compute_and_get_share(div(input_share_b));

    std::array<::asio::mutable_buffer, 3> buffers {
        ::asio::buffer(prog.bld.data(), sizeof(data_array_t)),
        ::asio::buffer(&offset_share_a, sizeof(input_type_1)),
        ::asio::buffer(&offset_share_b, sizeof(input_type_1))
    };

    std::size_t bytes_written = ::asio::write(peer, buffers, error);
    if (error)
    {
        return std::make_tuple(offset_share_a, offset_share_b, bytes_written, std::size_t(0));
    }
    std::size_t bytes_read = ::asio::read(peer, buffers, error);
    if (!error)
    {
        offset_share_a = dpf_a_oob.offset_x.reconstruct(offset_share_a);
        offset_share_b = dpf_b_oob.offset_x.reconstruct(offset_share_b);
        dpf_a.offset_x.set(static_cast<input_type_0>(offset_share_a));
        dpf_b.offset_x.set(static_cast<input_type_0>(offset_share_b));
    }

    return std::make_tuple(offset_share_a, offset_share_b, bytes_written, bytes_read);
}

template <typename PeerT,
          typename DataT,
          bool Party,
          std::size_t DivBit,
          typename DpfKey0,
          typename DpfKey1>
auto check_out_of_bounds_and_assign_wildcard_input_2_with_oob(PeerT & peer,
    const subleq_div<Party, DivBit, DataT> & div, const bool my_bool,
    DpfKey0 & dpf_a, DpfKey1 & dpf_a_oob, DataT input_share_a,
    DpfKey0 & dpf_b, DpfKey1 & dpf_b_oob, DataT input_share_b,
    ::asio::error_code & error)
{
    using input_type_0 = typename DpfKey0::input_type;
    using input_type_1 = typename DpfKey1::input_type;

    bool peer_bool;

    input_type_1 offset_share_a = dpf_a_oob.offset_x.compute_and_get_share(div(input_share_a));
    input_type_1 offset_share_b = dpf_b_oob.offset_x.compute_and_get_share(div(input_share_b));

    std::size_t bytes_written = ::asio::write(peer,
        std::array<::asio::const_buffer, 3> {
            ::asio::buffer(&my_bool, sizeof(bool)),
            ::asio::buffer(&offset_share_a, sizeof(input_type_1)),
            ::asio::buffer(&offset_share_b, sizeof(input_type_1))
        }, error);
    if (error)
    {
        return std::make_tuple(my_bool, offset_share_a, offset_share_b, bytes_written, std::size_t(0));
    }
    std::size_t bytes_read = ::asio::read(peer,
        std::array<::asio::mutable_buffer, 3> {
            ::asio::buffer(&peer_bool, sizeof(bool)),
            ::asio::buffer(&offset_share_a, sizeof(input_type_1)),
            ::asio::buffer(&offset_share_b, sizeof(input_type_1))
        }, error);
    if (!error)
    {
        offset_share_a = dpf_a_oob.offset_x.reconstruct(offset_share_a);
        offset_share_b = dpf_b_oob.offset_x.reconstruct(offset_share_b);
        dpf_a.offset_x.set(static_cast<input_type_0>(offset_share_a));
        dpf_b.offset_x.set(static_cast<input_type_0>(offset_share_b));
    }

    return std::make_tuple(!(my_bool ^ peer_bool), offset_share_a, offset_share_b, bytes_written, bytes_read);
}

template <typename PeerT,
          typename DataT,
          typename NodeT,
          bool Party,
          std::size_t DivBit,
          std::size_t DataSize,
          std::size_t DataPerCode,
          typename DpfKey0,
          typename DpfKey1>
auto assign_wildcard_input_1_with_oob(PeerT & peer,
    const subleq_div<Party, DivBit, DataT> & div,
    const subleq_dot_prod<DataSize, DataPerCode, DataT, NodeT> & dot_prod,
    DpfKey0 & dpf, DpfKey1 & dpf_oob, DataT input_share,
    const DataT w, DataT & bld_w, ::asio::error_code & error)
{
    using data_t = DataT;

    using input_type_0 = typename DpfKey0::input_type;
    using input_type_1 = typename DpfKey1::input_type;

    bld_w = w + dot_prod.blind_w;

    input_type_1 offset_share = dpf_oob.offset_x.compute_and_get_share(div(input_share));

    std::array<::asio::mutable_buffer, 3> buffers {
        ::asio::buffer(&bld_w, sizeof(data_t)),
        ::asio::buffer(&offset_share, sizeof(input_type_1))
    };

    std::size_t bytes_written = ::asio::write(peer, buffers, error);
    if (error)
    {
        return std::make_tuple(offset_share, bytes_written, std::size_t(0));
    }
    std::size_t bytes_read = ::asio::read(peer, buffers, error);
    if (!error)
    {
        offset_share = dpf_oob.offset_x.reconstruct(offset_share);
        dpf.offset_x.set(static_cast<input_type_0>(offset_share));
    }

    return std::make_tuple(offset_share, bytes_written, bytes_read);
}

template <typename InteriorPRG = dpf::prg::aes128,
          typename ExteriorPRG = InteriorPRG,
          typename PeerT,
          typename InputT,
          typename OutputT,
          typename ...OutputTs>
auto make_dpf_ret_input(PeerT & peer0, PeerT & peer1, std::size_t count, dpf::dpfargs<InputT, OutputT, OutputTs...> & args, ::asio::error_code & error, dpf::root_sampler_t<InteriorPRG> && root_sampler = dpf::uniform_sample<typename InteriorPRG::block_type>)
{
    using dpf_type = dpf::utils::dpf_type_t<InteriorPRG, ExteriorPRG, InputT, OutputT, OutputTs...>;
    using correction_words_array = typename dpf_type::correction_words_array;
    using correction_advice_array = typename dpf_type::correction_advice_array;
    using interior_node = typename dpf_type::interior_node;
    using leaf_tuple = typename dpf_type::leaf_tuple;
    using beaver_tuple = typename dpf_type::beaver_tuple;
    using input_type = typename dpf_type::input_type;

    std::vector<std::tuple<input_type, input_type, input_type>> xs;
    xs.reserve(count);

    std::size_t bytes_written0 = 0, bytes_written1 = 0;
    for (std::size_t num_written = 0; num_written < count; ++num_written)
    {
        auto [correction_words, correction_advice, priv0, priv1, x]
            = make_dpf_impl_ret_input<InteriorPRG, ExteriorPRG>(args, std::forward<dpf::root_sampler_t<InteriorPRG>>(root_sampler));
        auto & [root0, leaves0, beavers0, offset_share0] = priv0;
        auto & [root1, leaves1, beavers1, offset_share1] = priv1;
        xs.push_back(x);

        bytes_written0 += ::asio::write(peer0,
            std::array<::asio::const_buffer, 6>{
                ::asio::buffer(&correction_words,  sizeof(correction_words_array)),
                ::asio::buffer(&correction_advice, sizeof(correction_advice_array)),
                ::asio::buffer(&root0,             sizeof(interior_node)),
                ::asio::buffer(&leaves0,           sizeof(leaf_tuple)),
                ::asio::buffer(&beavers0,          sizeof(beaver_tuple)),
                ::asio::buffer(&offset_share0,     sizeof(input_type))
            }, error);
        if (error)
        {
            return std::make_tuple(xs, bytes_written0, bytes_written1, num_written);
        }
        
        bytes_written1 += ::asio::write(peer1,
            std::array<::asio::const_buffer, 6>{
                ::asio::buffer(&correction_words,  sizeof(correction_words_array)),
                ::asio::buffer(&correction_advice, sizeof(correction_advice_array)),
                ::asio::buffer(&root1,             sizeof(interior_node)),
                ::asio::buffer(&leaves1,           sizeof(leaf_tuple)),
                ::asio::buffer(&beavers1,          sizeof(beaver_tuple)),
                ::asio::buffer(&offset_share1,     sizeof(input_type))
            }, error);
        if (error)
        {
            return std::make_tuple(xs, bytes_written0, bytes_written1, num_written);
        }
    }
    return std::make_tuple(xs, bytes_written0, bytes_written1, count);
}

template <typename InteriorPRG = dpf::prg::aes128,
          typename ExteriorPRG = InteriorPRG,
          typename AltInputT,
          typename PeerT,
          typename InputT,
          typename OutputT,
          typename ...OutputTs>
auto make_dpf_set_input(PeerT & peer0, PeerT & peer1, std::size_t count, const std::vector<std::tuple<AltInputT, AltInputT, AltInputT>> & xs, dpf::dpfargs<InputT, OutputT, OutputTs...> & args, ::asio::error_code & error, dpf::root_sampler_t<InteriorPRG> && root_sampler = dpf::uniform_sample<typename InteriorPRG::block_type>)
{
    using dpf_type = dpf::utils::dpf_type_t<InteriorPRG, ExteriorPRG, InputT, OutputT, OutputTs...>;
    using correction_words_array = typename dpf_type::correction_words_array;
    using correction_advice_array = typename dpf_type::correction_advice_array;
    using interior_node = typename dpf_type::interior_node;
    using leaf_tuple = typename dpf_type::leaf_tuple;
    using beaver_tuple = typename dpf_type::beaver_tuple;
    using input_type = typename dpf_type::input_type;

    std::size_t bytes_written0 = 0, bytes_written1 = 0;
    for (std::size_t num_written = 0; num_written < count; ++num_written)
    {
        auto [correction_words, correction_advice, priv0, priv1]
            = make_dpf_impl_set_input<InteriorPRG, ExteriorPRG>(xs[num_written], args, std::forward<dpf::root_sampler_t<InteriorPRG>>(root_sampler));
        auto & [root0, leaves0, beavers0, offset_share0] = priv0;
        auto & [root1, leaves1, beavers1, offset_share1] = priv1;

        bytes_written0 += ::asio::write(peer0,
            std::array<::asio::const_buffer, 6>{
                ::asio::buffer(&correction_words,  sizeof(correction_words_array)),
                ::asio::buffer(&correction_advice, sizeof(correction_advice_array)),
                ::asio::buffer(&root0,             sizeof(interior_node)),
                ::asio::buffer(&leaves0,           sizeof(leaf_tuple)),
                ::asio::buffer(&beavers0,          sizeof(beaver_tuple)),
                ::asio::buffer(&offset_share0,     sizeof(input_type))
            }, error);
        if (error)
        {
            return std::make_tuple(bytes_written0, bytes_written1, num_written);
        }
        
        bytes_written1 += ::asio::write(peer1,
            std::array<::asio::const_buffer, 6>{
                ::asio::buffer(&correction_words,  sizeof(correction_words_array)),
                ::asio::buffer(&correction_advice, sizeof(correction_advice_array)),
                ::asio::buffer(&root1,             sizeof(interior_node)),
                ::asio::buffer(&leaves1,           sizeof(leaf_tuple)),
                ::asio::buffer(&beavers1,          sizeof(beaver_tuple)),
                ::asio::buffer(&offset_share1,     sizeof(input_type))
            }, error);
        if (error)
        {
            return std::make_tuple(bytes_written0, bytes_written1, num_written);
        }
    }
    return std::make_tuple(bytes_written0, bytes_written1, count);
}

}  // namespace asio

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_ASIO_HPP__
