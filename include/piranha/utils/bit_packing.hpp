// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_UTILS_BIT_PACKING_HPP
#define PIRANHA_UTILS_BIT_PACKING_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/ignore.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/visibility.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/type_traits.hpp>

// NOTE:
// - in these classes, we are exploiting two's complement representation when dealing with signed
//   integers. This is not guaranteed by the standard before C++20, but in practice even before
//   C++20 essentially all C++ implementations support only two's complement;
// - we have a few integral divisions/modulo operations in these classes which could probably
//   be replaced with lookup tables, should the need arise in terms of performance.

namespace piranha
{

// Only allow certain integral types to be packable (this is due to the complications arising
// from integral promotion rules for short ints and char types).
template <typename T>
using is_bit_packable = ::std::disjunction<::std::is_same<T, int>, ::std::is_same<T, unsigned>, ::std::is_same<T, long>,
                                           ::std::is_same<T, unsigned long>, ::std::is_same<T, long long>,
                                           ::std::is_same<T, unsigned long long>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                           ,
                                           ::std::is_same<T, __int128_t>, ::std::is_same<T, __uint128_t>
#endif
                                           >;

template <typename T>
inline constexpr bool is_bit_packable_v = is_bit_packable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL BitPackable = is_bit_packable_v<T>;

#endif

namespace detail
{

// Implementation details for the signed/unsigned packers.
template <typename T>
class signed_bit_packer_impl
{
public:
    constexpr explicit signed_bit_packer_impl(unsigned size)
        : m_value(0), m_min(0), m_max(0), m_index(0), m_size(size), m_pbits(0), m_cur_shift(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T> + 1);
        if (piranha_unlikely(size >= nbits)) {
            piranha_throw(::std::overflow_error,
                          "The size of a signed bit packer must be smaller than the bit width of the integral type ("
                              + detail::to_string(nbits) + "), but a size of " + detail::to_string(size)
                              + " was specified");
        }

        if (size) {
            if (size == 1u) {
                // Special case size 1 (use the full range of the type).
                m_pbits = nbits;
                ::std::tie(m_min, m_max) = detail::limits_minmax<T>;
            } else {
                // In the general case we cannot use the full bit width,
                // and we need at least one extra bit. Otherwise, we run
                // into overflow errors during packing.
                m_pbits = nbits / size - static_cast<unsigned>(nbits % size == 0u);
                assert(m_pbits);
                // Compute the limits.
                m_min = -(T(1) << (m_pbits - 1u));
                m_max = (T(1) << (m_pbits - 1u)) - T(1);
            }
        }
    }
    constexpr void operator<<(const T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot push any more values to this signed bit packer: the number of "
                          "values already pushed to the packer is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        if (piranha_unlikely(n < m_min || n > m_max)) {
            piranha_throw(::std::overflow_error,
                          "Cannot push the value " + detail::to_string(n)
                              + " to this signed bit packer: the value is outside the allowed range ["
                              + detail::to_string(m_min) + ", " + detail::to_string(m_max) + "]");
        }

        // NOTE: don't bit shift directly a signed value as,
        // before C++20, this is implementation-defined behaviour.
        // Also, go through a separate tmp variable because otherwise
        // some compilers are too eager in transforming this operation
        // into a direct bit shift, and this causes problems in constexpr
        // contexts.
        const auto tmp = T(1) << m_cur_shift;
        m_value += n * tmp;
        ++m_index;
        m_cur_shift += m_pbits;
    }
    constexpr const T &get() const
    {
        if (piranha_unlikely(m_index < m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot fetch the packed value from this signed bit packer: the number of "
                          "values pushed to the packer ("
                              + detail::to_string(m_index) + ") is less than the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }
        return m_value;
    }

private:
    T m_value, m_min, m_max;
    unsigned m_index, m_size, m_pbits, m_cur_shift;
};

template <typename T>
class unsigned_bit_packer_impl
{
public:
    constexpr explicit unsigned_bit_packer_impl(unsigned size)
        : m_value(0), m_max(0), m_index(0), m_size(size), m_pbits(0), m_cur_shift(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T>);
        if (piranha_unlikely(size > nbits)) {
            piranha_throw(
                ::std::overflow_error,
                "The size of an unsigned bit packer must not be larger than the bit width of the integral type ("
                    + detail::to_string(nbits) + "), but a size of " + detail::to_string(size) + " was specified");
        }

        if (size) {
            // NOTE: compute these values only if we are actually going
            // to pack values (if size is zero, no packing ever happens).
            //
            // m_pbits is the number of bits that can be used by each
            // packed value.
            m_pbits = nbits / size;
            // m_max is the maximum packed value (in unsigned format).
            // It is a sequence of m_pbits 1 bits.
            m_max = static_cast<T>(-1) >> (nbits - m_pbits);
        }
    }
    constexpr void operator<<(const T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot push any more values to this unsigned bit packer: the number of "
                          "values already pushed to the packer is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        if (piranha_unlikely(n > m_max)) {
            piranha_throw(::std::overflow_error,
                          "Cannot push the value " + detail::to_string(n)
                              + " to this unsigned bit packer: the value is outside the allowed range [9, "
                              + detail::to_string(m_max) + "]");
        }

        // Do the actual packing (the new value will be
        // appended in the MSB direction).
        m_value += n << m_cur_shift;
        ++m_index;
        m_cur_shift += m_pbits;
    }
    constexpr const T &get() const
    {
        if (piranha_unlikely(m_index < m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot fetch the packed value from this unsigned bit packer: the number of "
                          "values pushed to the packer ("
                              + detail::to_string(m_index) + ") is less than the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }
        return m_value;
    }

private:
    T m_value, m_max;
    unsigned m_index, m_size, m_pbits, m_cur_shift;
};

} // namespace detail

#if defined(PIRANHA_HAVE_CONCEPTS)
template <BitPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_bit_packable_v<T>>>
#endif
class bit_packer
{
    using impl_t
        = ::std::conditional_t<is_signed_v<T>, detail::signed_bit_packer_impl<T>, detail::unsigned_bit_packer_impl<T>>;

public:
    constexpr explicit bit_packer(unsigned size) : m_impl(size) {}
    constexpr bit_packer &operator<<(const T &n)
    {
        m_impl << n;
        return *this;
    }
    constexpr const T &get() const
    {
        return m_impl.get();
    }

private:
    impl_t m_impl;
};

namespace detail
{

// Helper to compute the min/max packed values for a signed integral T
// and for all the possible packer sizes.
template <typename T>
constexpr auto sbp_compute_minmax_packed()
{
    constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T> + 1);

    // Init the return value. The max size is the bit width of T minus 1 (which
    // corresponds to the number of binary digits given by std::numeric_limits).
    static_assert(static_cast<unsigned>(detail::limits_digits<T>)
                      <= ::std::get<1>(detail::limits_minmax<::std::size_t>),
                  "Overflow error.");
    ::std::array<::std::array<T, 2>, static_cast<unsigned>(detail::limits_digits<T>)> retval{};

    // For size 1, we have the special case of using the full range.
    retval[0][0] = ::std::get<0>(detail::limits_minmax<T>);
    retval[0][1] = ::std::get<1>(detail::limits_minmax<T>);

    // Build the remaining sizes.
    for (auto i = 1u; i < retval.size(); ++i) {
        // Pack vectors of min/max values for this size.
        const auto size = i + 1u;
        bit_packer<T> bp_min(size), bp_max(size);
        const auto pbits = nbits / size - static_cast<unsigned>(nbits % size == 0u);
        const auto min = -(T(1) << (pbits - 1u)), max = (T(1) << (pbits - 1u)) - T(1);
        for (auto j = 0u; j < size; ++j) {
            bp_min << min;
            bp_max << max;
        }
        // Extract the packed values.
        retval[i][0] = bp_min.get();
        retval[i][1] = bp_max.get();
    }

    return retval;
}

// Handy alias.
template <typename T>
using sbp_minmax_packed_t = decltype(sbp_compute_minmax_packed<T>());

// Declare the variables holding the min/max packed values for the
// supported signed integral types.
PIRANHA_PUBLIC extern const sbp_minmax_packed_t<int> sbp_mmp_int;
PIRANHA_PUBLIC extern const sbp_minmax_packed_t<long> sbp_mmp_long;
PIRANHA_PUBLIC extern const sbp_minmax_packed_t<long long> sbp_mmp_long_long;

#if defined(PIRANHA_HAVE_GCC_INT128)

PIRANHA_PUBLIC extern const sbp_minmax_packed_t<__int128_t> sbp_mmp_int128;

#endif

// Small generic wrapper to access the above constants.
template <typename T>
inline const auto &sbp_get_mmp()
{
    if constexpr (::std::is_same_v<T, int>) {
        return sbp_mmp_int;
    } else if constexpr (::std::is_same_v<T, long>) {
        return sbp_mmp_long;
    } else if constexpr (::std::is_same_v<T, long long>) {
        return sbp_mmp_long_long;
    }
#if defined(PIRANHA_HAVE_GCC_INT128)
    else if constexpr (::std::is_same_v<T, __int128_t>) {
        return sbp_mmp_int128;
    }
#endif
}

// Implementation details for the signed/unsigned unpackers.
template <typename T>
class signed_bit_unpacker_impl
{
    using uint_t = make_unsigned_t<T>;

public:
    constexpr explicit signed_bit_unpacker_impl(const T &n, unsigned size)
        : m_value(n), m_min(0), m_s_value(0), m_index(0), m_size(size), m_pbits(0), m_cur_shift(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T> + 1);
        if (piranha_unlikely(size >= nbits)) {
            piranha_throw(::std::overflow_error,
                          "The size of a signed bit unpacker must be smaller than the bit width of the integral type ("
                              + detail::to_string(nbits) + "), but a size of " + detail::to_string(size)
                              + " was specified");
        }

        if (size) {
            if (size == 1u) {
                // For unitary size, we leave everything set to zero
                // and we set m_min to n (after unsigned cast). Like this,
                // below in the unpacking we avoid excessive bit shifting
                // while still extracting back n after the single possible
                // unpacking.
                m_min = static_cast<uint_t>(n);
            } else {
                // Get the minimum/maximum values allowed for n
                const auto [min_n, max_n] = ::piranha::detail::sbp_get_mmp<T>()[size - 1u];

                // Range check for n.
                if (piranha_unlikely(n < min_n || n > max_n)) {
                    piranha_throw(::std::overflow_error,
                                  "The value " + detail::to_string(n) + " passed to a signed bit unpacker of size "
                                      + detail::to_string(size) + " is outside the allowed range ["
                                      + detail::to_string(min_n) + ", " + detail::to_string(max_n) + "]");
                }

                m_pbits = nbits / size - static_cast<unsigned>(nbits % size == 0u);
                m_min = static_cast<uint_t>(-(T(1) << (m_pbits - 1u)));
                // The shifted n value that will be used during unpacking.
                m_s_value = static_cast<uint_t>(n) - static_cast<uint_t>(min_n);
            }

        } else {
            if (piranha_unlikely(n)) {
                piranha_throw(::std::invalid_argument,
                              "Only a value of zero can be unpacked into an empty output range, but a value of "
                                  + detail::to_string(n) + " was provided instead");
            }
        }
    }
    constexpr void operator>>(T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot unpack any more values from this signed bit unpacker: the number of "
                          "values already unpacked is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        n = static_cast<T>((m_s_value % (uint_t(1) << (m_cur_shift + m_pbits))) / (uint_t(1) << m_cur_shift) + m_min);
        ++m_index;
        m_cur_shift += m_pbits;
    }

private:
    T m_value;
    uint_t m_min, m_s_value;
    unsigned m_index, m_size, m_pbits, m_cur_shift;
};

template <typename T>
class unsigned_bit_unpacker_impl
{
public:
    constexpr explicit unsigned_bit_unpacker_impl(const T &n, unsigned size)
        : m_value(n), m_mask(0), m_index(0), m_size(size), m_pbits(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T>);
        if (piranha_unlikely(size > nbits)) {
            piranha_throw(
                ::std::overflow_error,
                "The size of an unsigned bit unpacker cannot be larger than the bit width of the integral type ("
                    + detail::to_string(nbits) + "), but a size of " + detail::to_string(size) + " was specified");
        }

        if (size) {
            m_pbits = nbits / size;
            // The maximum decodable value is a sequence of m_pbits * size
            // one bits (starting from LSB).
            const auto max_decodable = T(-1) >> (nbits % size);
            if (piranha_unlikely(n > max_decodable)) {
                piranha_throw(::std::overflow_error,
                              "The value " + detail::to_string(n) + " passed to an unsigned bit unpacker of size "
                                  + detail::to_string(size) + " is outside the allowed range [0, "
                                  + detail::to_string(max_decodable) + "]");
            }
            // The mask for extracting the low m_pbits from a value.
            m_mask = T(-1) >> (nbits - m_pbits);

            // NOTE: if size == 1 we set m_pbits back to zero.
            // The reason is that for size == 1 we would end up
            // downshifting m_value by nbits in operator>>() below,
            // which is undefined behaviour. Note that m_pbits
            // is only used in the downshifting from this point
            // onwards.
            if (size == 1u) {
                m_pbits = 0;
            }
        } else {
            if (piranha_unlikely(n)) {
                piranha_throw(::std::invalid_argument,
                              "Only a value of zero can be unpacked into an empty output range, but a value of "
                                  + detail::to_string(n) + " was provided instead");
            }
        }
    }
    constexpr void operator>>(T &out)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot unpack any more values from this unsigned bit unpacker: the number of "
                          "values already unpacked is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        // Unpack the current value and write it out.
        out = m_value & m_mask;
        // Increase the index, shift down the current value.
        ++m_index;
        m_value >>= m_pbits;
    }

private:
    T m_value, m_mask;
    unsigned m_index, m_size, m_pbits;
};

} // namespace detail

#if defined(PIRANHA_HAVE_CONCEPTS)
template <BitPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_bit_packable_v<T>>>
#endif
class bit_unpacker
{
    using impl_t = ::std::conditional_t<is_signed_v<T>, detail::signed_bit_unpacker_impl<T>,
                                        detail::unsigned_bit_unpacker_impl<T>>;

public:
    constexpr explicit bit_unpacker(const T &n, unsigned size) : m_impl(n, size) {}
    constexpr bit_unpacker &operator>>(T &n)
    {
        m_impl >> n;
        return *this;
    }

private:
    impl_t m_impl;
};

} // namespace piranha

#endif
