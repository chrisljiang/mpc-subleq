#ifndef SUBLEQ_SUBLEQ_NEXT_ADDRESS_HPP__
#define SUBLEQ_SUBLEQ_NEXT_ADDRESS_HPP__

#include <array>
#include <tuple>
#include <utility>

#include "../libdpf/include/dpf/random.hpp"

namespace subleq
{

template <typename data_t>
struct subleq_next_address
{
  public:
    static constexpr std::size_t n_masks = 2;
    static constexpr std::size_t n_corrections = 3;

    using mask_array_t = std::array<data_t, n_masks>;
    using corrections_array_t = std::array<data_t, n_corrections>;

    subleq_next_address(bool party, mask_array_t mask_share_array, corrections_array_t correction_share_array)
      : party0(party), mask_shares(mask_share_array), masked_shares(), correction_shares(correction_share_array) { }
    subleq_next_address() = delete;
    subleq_next_address(const subleq_next_address &) = delete;
    subleq_next_address(subleq_next_address &&) = default;
    subleq_next_address & operator=(const subleq_next_address &) = delete;
    subleq_next_address & operator=(subleq_next_address &&) = default;

    bool party0;
    mask_array_t mask_shares;
    mask_array_t masked_shares;
    corrections_array_t correction_shares;

    template <typename PeerT>
    auto evaluate(PeerT & peer, const data_t parity, const data_t addr_c, const data_t addr_d, ::asio::error_code & error)
    {
        mask_array_t tmp_arr;

        masked_shares[0] = parity + mask_shares[0];
        masked_shares[1] = addr_c - addr_d + mask_shares[1];

        std::size_t bytes_written = ::asio::write(peer, ::asio::buffer(masked_shares.data(), sizeof(mask_array_t)), error);
        if (error)
        {
            return std::make_tuple(data_t(0), bytes_written, std::size_t(0));
        }
        std::size_t bytes_read = ::asio::read(peer, ::asio::buffer(tmp_arr.data(), sizeof(mask_array_t)), error);
        if (error)
        {
            return std::make_tuple(data_t(0), bytes_written, bytes_read);
        }

        for (std::size_t i = 0; i < n_masks; ++i)
        {
            masked_shares[i] += tmp_arr[i];
        }

        // masked_shares: 0-ss, 1-xx
        // mask_shares: 0-S, 1-X
        // correction_shares: 0-S^2, 1-S*X, 2-S^2*X
        // return [d] + ss^2*xx - ss^2*[X] - 2*ss*xx*[S] + 2*ss*[S*X] + xx*[S^2] - [S^2*X]
        return std::make_tuple(
            addr_d
                + masked_shares[0] * masked_shares[0] * masked_shares[1] * (party0 == true)
                - masked_shares[0] * masked_shares[0] * mask_shares[1]
                - 2 * masked_shares[0] * masked_shares[1] * mask_shares[0]
                + 2 * masked_shares[0] * correction_shares[1]
                + masked_shares[1] * correction_shares[0]
                - correction_shares[2],
            bytes_written,
            bytes_read
        );
    }

    static auto make_subleq_next_address()
    {
        mask_array_t masks, masks_0, masks_1;
        corrections_array_t corrections_0, corrections_1;

        dpf::uniform_fill(masks);          // Random masks
        dpf::uniform_fill(masks_0);        // Random masks
        dpf::uniform_fill(corrections_0);

        for(std::size_t i = 0; i < n_masks; ++i)
        {
            masks_1[i] = masks[i] - masks_0[i];
        }

        // [S], [X], [Y]
        // 0     1     2
        // [S^2]
        corrections_1[0] = (masks[0] * masks[0]) - corrections_0[0];
        // [S*X]
        corrections_1[1] = (masks[0] * masks[1]) - corrections_0[1];
        // [S^2*X]
        corrections_1[2] = (masks[0] * masks[0] * masks[1]) - corrections_0[2];

        return std::make_pair
        (
            subleq_next_address(true, masks_0, corrections_0),
            subleq_next_address(false, masks_1, corrections_1)
        );
    }

    template <typename PeerT>
    static auto make_next_address(PeerT & peer0, PeerT & peer1, std::size_t count, ::asio::error_code & error)
    {
        std::size_t bytes_written0 = 0,
                    bytes_written1 = 0;
        for (std::size_t num_written = 0; num_written < count; ++num_written)
        {
            auto [next0, next1] = make_subleq_next_address();

            bytes_written0 += ::asio::write(peer0,
                std::array<::asio::const_buffer, 3> {
                    ::asio::buffer(&next0.party0, sizeof(bool)),
                    ::asio::buffer(&next0.mask_shares, sizeof(mask_array_t)),
                    ::asio::buffer(&next0.correction_shares, sizeof(corrections_array_t))
                }, error);
            if (error)
            {
                return std::make_tuple(bytes_written0, bytes_written1, num_written);
            }

            bytes_written1 += ::asio::write(peer1,
                std::array<::asio::const_buffer, 3> {
                    ::asio::buffer(&next1.party0, sizeof(bool)),
                    ::asio::buffer(&next1.mask_shares, sizeof(mask_array_t)),
                    ::asio::buffer(&next1.correction_shares, sizeof(corrections_array_t))
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
    static auto read_next_address(PeerT & peer, BackEmplaceable & output, std::size_t count, ::asio::error_code & error)
    {
        bool party;
        mask_array_t mask_share_array;
        corrections_array_t correction_share_array;

        std::size_t bytes_read = 0;
        for (std::size_t num_read = 0; num_read < count; ++num_read)
        {
            bytes_read += ::asio::read(peer,
                std::array<::asio::mutable_buffer, 3> {
                    ::asio::buffer(&party, sizeof(bool)),
                    ::asio::buffer(&mask_share_array, sizeof(mask_array_t)),
                    ::asio::buffer(&correction_share_array, sizeof(corrections_array_t))
                }, error);
            if (error)
            {
                return std::make_tuple(bytes_read, num_read);
            }

            output.emplace_back(party, mask_share_array, correction_share_array);
        }

        return std::make_tuple(bytes_read, count);
    }

};

}  // namespace subleq

#endif // SUBLEQ_SUBLEQ_NEXT_ADDRESS_HPP__
