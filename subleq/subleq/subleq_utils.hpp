#ifndef SUBLEQ_SUBLEQ_UTILS_HPP__
#define SUBLEQ_SUBLEQ_UTILS_HPP__

#include <cmath>

namespace subleq
{

constexpr std::size_t get_bit_count(std::size_t val)
{
    return std::ceil(std::log2(val));
}

constexpr std::size_t get_item_count(std::size_t val)
{
    return static_cast<std::size_t>(1) << get_bit_count(val);
}

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_UTILS_HPP__
