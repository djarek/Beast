//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_BUFFER_TRAITS_HPP
#define BOOST_BEAST_BUFFER_TRAITS_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/buffer_traits.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/beast/core/detail/static_const.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/config/workaround.hpp>
#include <boost/mp11/function.hpp>
#include <boost/assert.hpp>
#include <type_traits>

namespace boost {
namespace beast {

/** Determine if a list of types satisfy the <em>ConstBufferSequence</em> requirements.

    This metafunction is used to determine if all of the specified types
    meet the requirements for constant buffer sequences. This type alias
    will be `std::true_type` if each specified type meets the requirements,
    otherwise, this type alias will be `std::false_type`.

    @tparam BufferSequence A list of zero or more types to check. If this
    list is empty, the resulting type alias will be `std::true_type`.
*/
template<class... BufferSequence>
#if BOOST_BEAST_DOXYGEN
using is_const_buffer_sequence = __see_below__;
#else
using is_const_buffer_sequence = mp11::mp_all<
    net::is_const_buffer_sequence<
        typename std::decay<BufferSequence>::type>...>;
#endif

/** Determine if a list of types satisfy the <em>MutableBufferSequence</em> requirements.

    This metafunction is used to determine if all of the specified types
    meet the requirements for mutable buffer sequences. This type alias
    will be `std::true_type` if each specified type meets the requirements,
    otherwise, this type alias will be `std::false_type`.

    @tparam BufferSequence A list of zero or more types to check. If this
    list is empty, the resulting type alias will be `std::true_type`.
*/
template<class... BufferSequence>
#if BOOST_BEAST_DOXYGEN
using is_mutable_buffer_sequence = __see_below__;
#else
using is_mutable_buffer_sequence = mp11::mp_all<
    net::is_mutable_buffer_sequence<
        typename std::decay<BufferSequence>::type>...>;
#endif

/** Type alias for the underlying buffer type of a list of buffer sequence types.

    This metafunction is used to determine the underlying buffer type for
    a list of buffer sequence. The equivalent type of the alias will vary
    depending on the template type argument:

    @li If every type in the list is a <em>MutableBufferSequence</em>,
        the resulting type alias will be `net::mutable_buffer`, otherwise

    @li The resulting type alias will be `net::const_buffer`.

    @par Example
    The following code returns the first buffer in a buffer sequence,
    or generates a compilation error if the argument is not a buffer
    sequence:
    @code
    template <class BufferSequence>
    buffers_type <BufferSequence>
    buffers_front (BufferSequence const& buffers)
    {
        static_assert(
            net::is_const_buffer_sequence<BufferSequence>::value,
            "BufferSequence type requirements not met");
        auto const first = net::buffer_sequence_begin (buffers);
        if (first == net::buffer_sequence_end (buffers))
            return {};
        return *first;
    }
    @endcode

    @tparam BufferSequence A list of zero or more types to check. If this
    list is empty, the resulting type alias will be `net::mutable_buffer`.
*/
template<class... BufferSequence>
#if BOOST_BEAST_DOXYGEN
using buffers_type = __see_below__;
#else
using buffers_type = typename std::conditional<
    is_mutable_buffer_sequence<BufferSequence...>::value,
    net::mutable_buffer, net::const_buffer>::type;
#endif

/** Type alias for the iterator type of a buffer sequence type.

    This metafunction is used to determine the type of iterator
    used by a particular buffer sequence.

    @tparam T The buffer sequence type to use. The resulting
    type alias will be equal to the iterator type used by
    the buffer sequence.
*/
template <class BufferSequence>
#if BOOST_BEAST_DOXYGEN
using buffers_iterator_type = __see_below__;
#elif BOOST_WORKAROUND(BOOST_MSVC, < 1910)
using buffers_iterator_type = typename
    detail::buffers_iterator_type_helper<
        typename std::decay<BufferSequence>::type>::type;
#else
using buffers_iterator_type =
    decltype(net::buffer_sequence_begin(
        std::declval<BufferSequence const&>()));
#endif

/** Return the total number of bytes in a buffer or buffer sequence

    This function returns the total number of bytes in a buffer,
    buffer sequence, or object convertible to a buffer. Specifically
    it may be passed:

    @li A <em>ConstBufferSequence</em> or <em>MutableBufferSequence</em>

    @li A `net::const_buffer` or `net::mutable_buffer`

    @li An object convertible to `net::const_buffer`

    This function is designed as an easier-to-use replacement for
    `net::buffer_size`. It recognizes customization points found through
    argument-dependent lookup. The call `beast::buffer_bytes(b)` is
    equivalent to performing:
    @code
    using namespace net;
    buffer_bytes(b);
    @endcode
    In addition this handles types which are convertible to
    `net::const_buffer`; these are not handled by `net::buffer_size`.

    @param buffers The buffer or buffer sequence to calculate the size of.

    @return The total number of bytes in the buffer or sequence.
*/
#if BOOST_BEAST_DOXYGEN
template<class BufferSequence>
std::size_t
buffer_bytes(BufferSequence const& buffers);
#else
BOOST_BEAST_INLINE_VARIABLE(buffer_bytes, detail::buffer_bytes_impl)
#endif

#ifndef BOOST_BEAST_DOXYGEN
namespace detail {
struct dynamic_buffer_access;
} // detail;
#endif

/** A DynamicBuffer adaptor for storage objects.
*/
template<class Storage>
class dynamic_storage_buffer
{
    Storage& ds_;
    std::size_t max_size_;

    friend struct detail::dynamic_buffer_access;

    explicit
    dynamic_storage_buffer(
        Storage& storage,
        std::size_t max_size)
        : ds_(storage)
        , max_size_(max_size)
    {
    }

public:
    /// The ConstBufferSequence used to represent the readable bytes.
    using const_buffers_type =
        typename Storage::const_buffers_type;

    /// The MutableBufferSequence used to represent the writable bytes.
    using mutable_buffers_type =
        typename Storage::mutable_buffers_type;

    dynamic_storage_buffer(dynamic_storage_buffer const&) = default;

    dynamic_storage_buffer*
    operator->() noexcept
    {
        return this;
    }

    /// Returns the number of readable bytes.
    std::size_t
    size() const noexcept
    {
        return ds_.size();
    }

    /// Return the maximum number of bytes, both readable and writable, that can ever be held.
    std::size_t
    max_size() const noexcept
    {
        return std::min<std::size_t>(
            max_size_, ds_.max_size());
    }

    /// Return the maximum number of bytes, both readable and writable, that can be held without requiring an allocation.
    std::size_t
    capacity() const noexcept
    {
        return ds_.capacity();
    }

#ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1

    /// Returns a constant buffer sequence representing the readable bytes
    const_buffers_type
    data() const noexcept
    {
        return ds_.data();
    }

    /// Returns a mutable buffer sequence representing the readable bytes
    mutable_buffers_type
    data() noexcept
    {
        return ds_.data();
    }

    /** Returns a mutable buffer sequence representing writable bytes.
    
        Returns a mutable buffer sequence representing the writable
        bytes containing exactly `n` bytes of storage. Memory may be
        reallocated as needed.

        All buffers sequences previously obtained using
        @ref data or @ref prepare become invalid.

        @param n The desired number of bytes in the returned buffer
        sequence.

        @throws std::length_error if `size() + n` exceeds either
        `max_size()` or the allocator's maximum allocation size.

        @esafe

        Strong guarantee.
    */
    mutable_buffers_type
    prepare(std::size_t n)
    {
        if(detail::sum_exceeds(
            ds_.size(), n,
            std::min<std::size_t>(
                ds_.max_size(),
                max_size_)))
            BOOST_THROW_EXCEPTION(std::length_error(
                "dynamic buffer overflow"));
        return ds_.prepare(n);
    }

    /** Append writable bytes to the readable bytes.

        Appends n bytes from the start of the writable bytes to the
        end of the readable bytes. The remainder of the writable bytes
        are discarded. If n is greater than the number of writable
        bytes, all writable bytes are appended to the readable bytes.

        All buffers sequences previously obtained using
        @ref data or @ref prepare become invalid.

        @param n The number of bytes to append. If this number
        is greater than the number of writable bytes, all
        writable bytes are appended.

        @esafe

        No-throw guarantee.
    */
    void
    commit(std::size_t n) noexcept
    {
        ds_.commit(n);
    }

#endif

    /** Return a constant buffer sequence representing the underlying memory.

        The returned buffer sequence `u` represents the underlying
        memory beginning at offset `pos` and where `buffer_size(u) <= n`.

        @param pos The offset to start from. If this is larger than
        the size of the underlying memory, an empty buffer sequence
        is returned.

        @param n The maximum number of bytes in the returned sequence,
        starting from `pos`.

        @return The constant buffer sequence
    */
    const_buffers_type
    data(std::size_t pos, std::size_t n) const noexcept
    {
        return ds_.data(pos, n);
    }

    /** Return a mutable buffer sequence representing the underlying memory.

        The returned buffer sequence `u` represents the underlying
        memory beginning at offset `pos` and where `buffer_size(u) <= n`.

        @param pos The offset to start from. If this is larger than
        the size of the underlying memory, an empty buffer sequence
        is returned.

        @param n The maximum number of bytes in the returned sequence,
        starting from `pos`.

        @return The mutable buffer sequence
    */
    mutable_buffers_type
    data(std::size_t pos, std::size_t n) noexcept
    {
        return ds_.data(pos, n);
    }

    /** Extend the underlying memory to accommodate additional bytes.

        @param n The number of additional bytes to extend by.

        @throws `length_error` if `size() + n > max_size()`.
    */
    void
    grow(std::size_t n)
    {
        if(detail::sum_exceeds(
            ds_.size(), n,
            std::min<std::size_t>(
                ds_.max_size(),
                max_size_)))
            BOOST_THROW_EXCEPTION(std::length_error(
                "dynamic buffer overflow"));
        ds_.grow(n);
    }

    /** Remove bytes from the end of the underlying memory.

        This removes bytes from the end of the underlying memory. If
        the number of bytes to remove is larger than `size()`, then
        all underlying memory is emptied.

        @param n The number of bytes to remove.
    */
    void
    shrink(std::size_t n)
    {
        ds_.shrink(n);
    }

    /** Remove bytes from beginning of the readable bytes.

        Removes n bytes from the beginning of the readable bytes.

        All buffers sequences previously obtained using
        @ref data or @ref prepare become invalid.

        @param n The number of bytes to remove. If this number
        is greater than the number of readable bytes, all
        readable bytes are removed.

        @esafe

        No-throw guarantee.
    */
    void
    consume(std::size_t n) noexcept
    {
        ds_.consume(n);
    }
};

class dynamic_preparation
{
    std::size_t original_size_ = 0;
    std::size_t grow_by_ = 0;

public:
    template<class DynamicBuffer>
    static
    std::size_t
    suggested_growth(
        DynamicBuffer const& buffer,
        std::size_t upper_limit = 1536,
        std::size_t lower_limit = 512) noexcept
    {
        return std::min<std::size_t>(
            std::min<std::size_t>(
                upper_limit,
                buffer.max_size() - buffer.size()),
            std::max<std::size_t>(
                lower_limit,
                buffer.capacity() - buffer.size()));
    }

    dynamic_preparation() = default;

    template<class DynamicBuffer>
    explicit
    dynamic_preparation(
        DynamicBuffer const& buffer) noexcept
        : original_size_(buffer.size())
    {
    }

    std::size_t
    size() const noexcept
    {
        return grow_by_;
    }

    template<class DynamicBuffer>
    void
    grow(
        DynamicBuffer& buffer,
        std::size_t upper_limit = 1536,
        std::size_t lower_limit = 512)
    {
        original_size_ = buffer.size();
        grow_by_ =  suggested_growth(
            buffer, upper_limit, lower_limit);
        buffer.grow(grow_by_);
    }

    template<class DynamicBuffer>
    typename DynamicBuffer::mutable_buffers_type
    data(DynamicBuffer& buffer) const
    {
        return buffer.data(original_size_, grow_by_);
    }

    template<class DynamicBuffer>
    void
    commit(
        DynamicBuffer& buffer,
        std::size_t bytes_transferred)
    {
        BOOST_ASSERT(bytes_transferred <= grow_by_);
        buffer.shrink(grow_by_ - bytes_transferred);
        original_size_ = buffer.size();
        grow_by_ = 0;
    }
};

#ifndef  BOOST_BEAST_DOXYGEN
namespace detail {
struct dynamic_buffer_access
{
    template<class Storage>
    static
    dynamic_storage_buffer<Storage>
    make_dynamic_buffer(
        Storage& storage,
        std::size_t max_size =
        (std::numeric_limits<std::size_t>::max)())
    {
        return dynamic_storage_buffer<
            Storage>(storage, max_size);
    }
};
template<class T>
struct is_dynamic_buffer_v2 : std::false_type
{
};
template<class Storage>
struct is_dynamic_buffer_v2<
        dynamic_storage_buffer<Storage>>
    : std::true_type
{
};
} // detail
#endif

} // beast
} // boost

#endif
