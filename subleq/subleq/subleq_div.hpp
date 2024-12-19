#ifndef SUBLEQ_SUBLEQ_DIV_HPP__
#define SUBLEQ_SUBLEQ_DIV_HPP__

template <bool Party0,
          std::size_t DivBit,
          typename data_t>
struct subleq_div
{
    data_t operator()(data_t val) const
    {
        data_t ret = val >> DivBit;
        if constexpr(Party0 == true)
        {
            // If least DivBit-bits are non-zero, then add 1 to ret
            ret += static_cast<data_t>(static_cast<bool>(val % (1 << DivBit)));
        }
        return ret;
    }
};

#endif // SUBLEQ_SUBLEQ_DIV_HPP__
