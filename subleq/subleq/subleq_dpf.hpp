#ifndef SUBLEQ_SUBLEQ_DPF_HPP__
#define SUBLEQ_SUBLEQ_DPF_HPP__

#include "../libdpf/include/dpf/dpf_key.hpp"

namespace subleq
{

template <typename InteriorPRG,
          typename ExteriorPRG,
          typename InputT,
          typename OutputT,
          typename ...OutputTs>
auto make_dpf_impl_ret_input(dpf::dpfargs<InputT, OutputT, OutputTs...> args, dpf::root_sampler_t<InteriorPRG> && root_sampler = dpf::uniform_sample<typename InteriorPRG::block_type>)
{
    using dpf_type = dpf::utils::dpf_type_t<InteriorPRG, ExteriorPRG, InputT,
                                       OutputT, OutputTs...>;
    using interior_node = typename dpf_type::interior_node;
    using input_type = typename dpf_type::input_type;
    using correction_words_array = typename dpf_type::correction_words_array;
    using correction_advice_array = typename dpf_type::correction_advice_array;

    constexpr auto depth = dpf_type::depth;
    auto mask = dpf_type::msb_mask;

    static_assert(dpf::is_wildcard_v<InputT>, "input type must be wildcard to use this function");

    input_type x, x0{}, x1{};
    std::tie(x, x0, x1) = args.x();

    dpf::utils::flip_msb_if_signed_integral(x);

    const interior_node root[2] = {
        dpf::unset_lo_bit(root_sampler()),
        dpf::set_lo_bit(root_sampler())
    };

HEDLEY_PRAGMA(GCC diagnostic push)
HEDLEY_PRAGMA(GCC diagnostic ignored "-Wignored-attributes")
    correction_words_array correction_words;
HEDLEY_PRAGMA(GCC diagnostic pop)
    correction_advice_array correction_advice;

    interior_node parent[2] = { root[0], root[1] };
    bool advice[2];

    for (std::size_t level = 0; level < depth; ++level, mask >>= 1)
    {
        bool bit = !!(mask & x);

        advice[0] = dpf::get_lo_bit_and_clear_lo_2bits(parent[0]);
        advice[1] = dpf::get_lo_bit_and_clear_lo_2bits(parent[1]);

        auto child0 = InteriorPRG::eval01(parent[0]);
        auto child1 = InteriorPRG::eval01(parent[1]);
        interior_node child[2] = {
            child0[0] ^ child1[0],
            child0[1] ^ child1[1]
        };

        bool t[2] = {
            static_cast<bool>(dpf::get_lo_bit(child[0]) ^ !bit),
            static_cast<bool>(dpf::get_lo_bit(child[1]) ^ bit)
        };
        auto cw = dpf::set_lo_bit(child[!bit], t[bit]);
        parent[0] = dpf::xor_if(child0[bit], cw, advice[0]);
        parent[1] = dpf::xor_if(child1[bit], cw, advice[1]);

        correction_words[level] = child[!bit];
        correction_advice[level] = static_cast<psnip_uint8_t>(t[1] << 1) | t[0];
    }

    bool sign0 = dpf::get_lo_bit(parent[0]);
    // bool sign1 = dpf::get_lo_bit(parent[1]);

    auto [pair0, pair1] = std::apply([&x, &parent, &sign0](auto && ...ys)
        {
            return dpf::make_leaves<ExteriorPRG>(x,
                                                 dpf::unset_lo_2bits(parent[0]),
                                                 dpf::unset_lo_2bits(parent[1]),
                                                 sign0, ys...); }, args.y);
    auto && [leaves0, beavers0] = pair0;
    auto && [leaves1, beavers1] = pair1;

    return std::make_tuple(correction_words, correction_advice,
        std::make_tuple(root[0], leaves0, beavers0, x0),
        std::make_tuple(root[1], leaves1, beavers1, x1),
        std::make_tuple(x, x0, x1));
}

template <typename InteriorPRG,
          typename ExteriorPRG,
          typename AltInputT,
          typename InputT,
          typename OutputT,
          typename ...OutputTs>
auto make_dpf_impl_set_input(std::tuple<AltInputT, AltInputT, AltInputT> alt_x, dpf::dpfargs<InputT, OutputT, OutputTs...> args, dpf::root_sampler_t<InteriorPRG> && root_sampler = dpf::uniform_sample<typename InteriorPRG::block_type>)
{
    using dpf_type = dpf::utils::dpf_type_t<InteriorPRG, ExteriorPRG, InputT,
                                       OutputT, OutputTs...>;
    using interior_node = typename dpf_type::interior_node;
    using input_type = typename dpf_type::input_type;
    using correction_words_array = typename dpf_type::correction_words_array;
    using correction_advice_array = typename dpf_type::correction_advice_array;

    constexpr auto depth = dpf_type::depth;
    auto mask = dpf_type::msb_mask;

    static_assert(dpf::is_wildcard_v<InputT>, "input type must be wildcard to use this function");

    input_type x{static_cast<input_type>(std::get<0>(alt_x))},
               x0{static_cast<input_type>(std::get<1>(alt_x))},
               x1{static_cast<input_type>(std::get<2>(alt_x))};

    dpf::utils::flip_msb_if_signed_integral(x);

    const interior_node root[2] = {
        dpf::unset_lo_bit(root_sampler()),
        dpf::set_lo_bit(root_sampler())
    };

HEDLEY_PRAGMA(GCC diagnostic push)
HEDLEY_PRAGMA(GCC diagnostic ignored "-Wignored-attributes")
    correction_words_array correction_words;
HEDLEY_PRAGMA(GCC diagnostic pop)
    correction_advice_array correction_advice;

    interior_node parent[2] = { root[0], root[1] };
    bool advice[2];

    for (std::size_t level = 0; level < depth; ++level, mask >>= 1)
    {
        bool bit = !!(mask & x);

        advice[0] = dpf::get_lo_bit_and_clear_lo_2bits(parent[0]);
        advice[1] = dpf::get_lo_bit_and_clear_lo_2bits(parent[1]);

        auto child0 = InteriorPRG::eval01(parent[0]);
        auto child1 = InteriorPRG::eval01(parent[1]);
        interior_node child[2] = {
            child0[0] ^ child1[0],
            child0[1] ^ child1[1]
        };

        bool t[2] = {
            static_cast<bool>(dpf::get_lo_bit(child[0]) ^ !bit),
            static_cast<bool>(dpf::get_lo_bit(child[1]) ^ bit)
        };
        auto cw = dpf::set_lo_bit(child[!bit], t[bit]);
        parent[0] = dpf::xor_if(child0[bit], cw, advice[0]);
        parent[1] = dpf::xor_if(child1[bit], cw, advice[1]);

        correction_words[level] = child[!bit];
        correction_advice[level] = static_cast<psnip_uint8_t>(t[1] << 1) | t[0];
    }

    bool sign0 = dpf::get_lo_bit(parent[0]);
    // bool sign1 = dpf::get_lo_bit(parent[1]);

    auto [pair0, pair1] = std::apply([&x, &parent, &sign0](auto && ...ys)
        {
            return dpf::make_leaves<ExteriorPRG>(x,
                                                 dpf::unset_lo_2bits(parent[0]),
                                                 dpf::unset_lo_2bits(parent[1]),
                                                 sign0, ys...); }, args.y);
    auto && [leaves0, beavers0] = pair0;
    auto && [leaves1, beavers1] = pair1;

    return std::make_tuple(correction_words, correction_advice,
        std::make_tuple(root[0], leaves0, beavers0, x0),
        std::make_tuple(root[1], leaves1, beavers1, x1));
}

template <typename InteriorPRG = dpf::prg::aes128,
          typename ExteriorPRG = InteriorPRG,
          typename InputT,
          typename OutputT = dpf::bit,
          typename ...OutputTs>
auto make_dpf_ret_input(dpf::dpfargs<InputT, OutputT, OutputTs...> args, dpf::root_sampler_t<InteriorPRG> && root_sampler = dpf::uniform_sample<typename InteriorPRG::block_type>)
{
    using dpf_type = dpf::utils::dpf_type_t<InteriorPRG, ExteriorPRG, InputT,
                                       OutputT, OutputTs...>;

    auto [correction_words, correction_advice,
          tuple0, tuple1, x] = make_dpf_impl_ret_input<InteriorPRG, ExteriorPRG>(args,
            std::forward<dpf::root_sampler_t<InteriorPRG>>(root_sampler));
    auto & [root0, leaves0, beavers0, offset0] = tuple0;
    auto & [root1, leaves1, beavers1, offset1] = tuple1;

    return std::make_tuple(
        dpf_type{root0, correction_words, correction_advice,
            leaves0, beavers0, offset0},
        dpf_type{root1, correction_words, correction_advice,
            leaves1, beavers1, offset1},
        x);
}

template <typename InteriorPRG = dpf::prg::aes128,
          typename ExteriorPRG = InteriorPRG,
          typename AltInputT,
          typename InputT,
          typename OutputT = dpf::bit,
          typename ...OutputTs>
auto make_dpf_set_input(std::tuple<AltInputT, AltInputT, AltInputT> alt_x, dpf::dpfargs<InputT, OutputT, OutputTs...> args, dpf::root_sampler_t<InteriorPRG> && root_sampler = dpf::uniform_sample<typename InteriorPRG::block_type>)
{
    using dpf_type = dpf::utils::dpf_type_t<InteriorPRG, ExteriorPRG, InputT,
                                       OutputT, OutputTs...>;

    auto [correction_words, correction_advice,
          tuple0, tuple1] = make_dpf_impl_set_input<InteriorPRG, ExteriorPRG>(alt_x, args,
            std::forward<dpf::root_sampler_t<InteriorPRG>>(root_sampler));
    auto & [root0, leaves0, beavers0, offset0] = tuple0;
    auto & [root1, leaves1, beavers1, offset1] = tuple1;

    return std::make_pair(
        dpf_type{root0, correction_words, correction_advice,
            leaves0, beavers0, offset0},
        dpf_type{root1, correction_words, correction_advice,
            leaves1, beavers1, offset1});
}

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_DPF_HPP__
