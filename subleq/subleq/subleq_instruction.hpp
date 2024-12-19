#ifndef SUBLEQ_SUBLEQ_INSTRUCTION_HPP__
#define SUBLEQ_SUBLEQ_INSTRUCTION_HPP__

#include <algorithm>
#include <array>

#include "simde/simde/x86/avx2.h"

namespace subleq
{

template <std::size_t data_per_code, typename data_t, typename node_t = std::array<data_t, data_per_code>>
union subleq_instruction
{
    static_assert(sizeof(node_t) >= sizeof(std::array<data_t, data_per_code>), "size of vec too small");
    static_assert(data_per_code == 3 || data_per_code == 4, "data per code must be 3 or 4");

    std::array<data_t, 4> addrs;
    node_t vec;

    subleq_instruction()
    {
        std::fill(std::begin(addrs), std::end(addrs), 0);
    };
    subleq_instruction(node_t v) : vec(v) { }

    data_t & operator[](std::size_t idx)
    {
        return addrs[idx];
    }

    template <typename data_array_t>
    void set(const data_array_t & data, data_t addr)
    {
        for (std::size_t i = 0; i < data_per_code; ++i)
        {
            addrs[i] = static_cast<data_t>(data[addr + i]);
        }
    }
};

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator*(
    data_t lhs, const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(rhs.addrs), std::end(rhs.addrs), std::begin(ret.addrs), [&lhs](const data_t & val)
    {
        return lhs * val;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator*(
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & lhs, data_t rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(lhs.addrs), std::end(lhs.addrs), std::begin(ret.addrs), [&rhs](const data_t & val)
    {
        return val * rhs;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator+(
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & lhs,
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(lhs.addrs), std::end(lhs.addrs), std::begin(rhs.addrs), std::begin(ret.addrs), [](const data_t & lhsval, const data_t & rhsval)
    {
        return lhsval + rhsval;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator+(
    data_t lhs, const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(rhs.addrs), std::end(rhs.addrs), std::begin(ret.addrs), [&lhs](const data_t & val)
    {
        return lhs + val;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator+(
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & lhs, data_t rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(lhs.addrs), std::end(lhs.addrs), std::begin(ret.addrs), [&rhs](const data_t & val)
    {
        return val + rhs;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator-(
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & lhs,
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(lhs.addrs), std::end(lhs.addrs), std::begin(rhs.addrs), std::begin(ret.addrs), [](const data_t & lhsval, const data_t & rhsval)
    {
        return lhsval - rhsval;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator-(
    data_t lhs, const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(rhs.addrs), std::end(rhs.addrs), std::begin(ret.addrs), [&lhs](const data_t & val)
    {
        return lhs - val;
    });
    return ret;
}

template<std::size_t data_per_code, typename data_t>
subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> operator-(
    const subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> & lhs, data_t rhs)
{
    subleq_instruction<data_per_code, data_t, std::array<data_t, data_per_code>> ret;
    std::transform(std::begin(lhs.addrs), std::end(lhs.addrs), std::begin(ret.addrs), [&rhs](const data_t & val)
    {
        return val - rhs;
    });
    return ret;
}

subleq_instruction<4, int32_t, simde__m128i> operator*(int32_t lhs, const subleq_instruction<4, int32_t, simde__m128i> & rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_mullo_epi32(simde_mm_set1_epi32(lhs), rhs.vec));
}

subleq_instruction<4, int32_t, simde__m128i> operator*(const subleq_instruction<4, int32_t, simde__m128i> & lhs, int32_t rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_mullo_epi32(lhs.vec, simde_mm_set1_epi32(rhs)));
}

subleq_instruction<4, int32_t, simde__m128i> operator+(const subleq_instruction<4, int32_t, simde__m128i> & lhs, const subleq_instruction<4, int32_t, simde__m128i> & rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_add_epi32(lhs.vec, rhs.vec));
}

subleq_instruction<4, int32_t, simde__m128i> operator+(int32_t lhs, const subleq_instruction<4, int32_t, simde__m128i> & rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_add_epi32(simde_mm_set1_epi32(lhs), rhs.vec));
}

subleq_instruction<4, int32_t, simde__m128i> operator+(const subleq_instruction<4, int32_t, simde__m128i> & lhs, int32_t rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_add_epi32(lhs.vec, simde_mm_set1_epi32(rhs)));
}

subleq_instruction<4, int32_t, simde__m128i> operator-(const subleq_instruction<4, int32_t, simde__m128i> & lhs, const subleq_instruction<4, int32_t, simde__m128i> & rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_sub_epi32(lhs.vec, rhs.vec));
}

subleq_instruction<4, int32_t, simde__m128i> operator-(int32_t lhs, const subleq_instruction<4, int32_t, simde__m128i> & rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_sub_epi32(simde_mm_set1_epi32(lhs), rhs.vec));
}

subleq_instruction<4, int32_t, simde__m128i> operator-(const subleq_instruction<4, int32_t, simde__m128i> & lhs, int32_t rhs)
{
    return subleq_instruction<4, int32_t, simde__m128i>(simde_mm_sub_epi32(lhs.vec, simde_mm_set1_epi32(rhs)));
}

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_INSTRUCTION_HPP__
