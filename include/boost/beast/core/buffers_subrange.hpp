//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_BUFFERS_SUBRANGE_HPP
#define BOOST_BEAST_BUFFERS_SUBRANGE_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/config/workaround.hpp>

namespace boost {
namespace beast {

/** A view representing a subrange of a buffer sequence.
*/
template<class BufferSequence>
class buffers_subrange
{
    using iter_type =
        buffers_iterator_type<BufferSequence>;

    BufferSequence b_;
    iter_type begin_;       // first buffer
    iter_type end_;         // one-past last buffer
    std::size_t d0_ = 0;
    std::size_t trim_;      // first buffer offset
    std::size_t chop_;      // last buffer size (before trim)
    std::size_t ord_ = 0;           // ordinal of end_

public:
#if BOOST_BEAST_DOXYGEN
    using value_type = __see_below__;
#elif BOOST_WORKAROUND(BOOST_MSVC, < 1910)
    using value_type = typename std::conditional<
        boost::is_convertible<typename
            std::iterator_traits<iter_type>::value_type,
                net::mutable_buffer>::value,
                    net::mutable_buffer,
                        net::const_buffer>::type;
#else
    using value_type = buffers_type<BufferSequence>;
#endif

#if BOOST_BEAST_DOXYGEN
    /// A bidirectional iterator type that may be used to read elements.
    using const_iterator = __implementation_defined__;

#else
    class const_iterator;
#endif

    /// Copy Constructor
    buffers_subrange(
        buffers_subrange const&);

    /// Copy Assignment
    buffers_subrange&
    operator=(buffers_subrange const&);

    /** Construct a consumable buffer sequence.
    */
    explicit
    buffers_subrange(BufferSequence const& buffers);

    /** Construct a prefix of a buffer sequence.

        @param n The maximum number of bytes in the returned sequence,
        starting from the beginning of the underlying buffers.

        @return The mutable buffer sequence
    */
    buffers_subrange(
        BufferSequence const& buffers,
        std::size_t n);

    /** Construct a subrange of a buffer sequence.

        @param pos The offset to start from. If this is larger than
        the size of the underlying buffers, an empty buffer sequence
        is returned.

        @param n The maximum number of bytes in the returned sequence,
        starting from offset `pos` in the underlying buffers.
    */
    buffers_subrange(
        BufferSequence const& buffers,
        std::size_t pos, std::size_t n);

    const_iterator
    begin() const noexcept;

    const_iterator
    end() const noexcept;

    /** Remove bytes from the beginning of the sequence.

        This function removes bytes start at the beginning of the current
        sequence. Only the view is changed, the underlying storage is not
        modified.

        @param n The number of bytes to remove. If this is larger than
        the number of bytes remaining, the view will become empty.
    */
    void
    consume(std::size_t n);
};

/** Return a view representing a subrange of a buffer sequence.

    @param pos The offset to start from. If this is larger than
    the size of the underlying buffers, an empty buffer sequence
    is returned.

    @param n The maximum number of bytes in the returned sequence,
    starting from offset `pos` in the underlying buffers.

    @return The buffer sequence. A copy of the original buffer
    sequence is maintained for the lifetime of the view.
*/
template<class BufferSequence>
auto
make_buffers_subrange(
    BufferSequence const& buffers,
    std::size_t pos, std::size_t n) ->
        buffers_subrange<BufferSequence>;

} // beast
} // boost

#include <boost/beast/core/impl/buffers_subrange.hpp>

#endif
