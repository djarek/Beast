//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_BUFFERS_SUBRANGE_HPP
#define BOOST_BEAST_IMPL_BUFFERS_SUBRANGE_HPP

#include <boost/assert.hpp>
#include <limits>

namespace boost {
namespace beast {

template<class BufferSequence>
class buffers_subrange<BufferSequence>::const_iterator
{
    buffers_subrange const* b_ = nullptr;
    iter_type it_ = {};
    std::size_t trim_ = 0;  // first buffer offset
    std::size_t chop_ = 0;  // last buffer size (before trim)
    std::size_t step_ = 0;   // index of it_ in range
    std::size_t ord_ = 0;   // one past last valid index

    friend class buffers_subrange;

    const_iterator(
        buffers_subrange const* b,
        iter_type it,
        std::size_t trim,
        std::size_t chop,
        std::size_t step,
        std::size_t ord)
        : b_(b)
        , it_(it)
        , trim_(trim)
        , chop_(chop)
        , step_(step)
        , ord_(ord)
    {
        BOOST_ASSERT(
            ord_ != 1 || chop_ >= trim_);
    }

public:
    using value_type = typename buffers_subrange<
        BufferSequence>::value_type;
    using pointer = value_type const*;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::bidirectional_iterator_tag;

    const_iterator() = default;
    const_iterator(const_iterator const&) = default;
    const_iterator& operator=(
        const_iterator const&) = default;

    pointer
    operator->() const = delete;

    bool
    operator==(const_iterator const& other) const
    {
        if(b_ != other.b_)
            return false;
        return it_ == other.it_;
    }

    bool
    operator!=(const_iterator const& other) const
    {
        if(b_ != other.b_)
            return true;
        return it_ != other.it_;
    }

    const_iterator&
    operator++()
    {
        BOOST_ASSERT(step_ < ord_);
        ++it_;
        ++step_;
        return *this;
    }

    const_iterator
    operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    const_iterator&
    operator--()
    {
        BOOST_ASSERT(step_ > 0);
        --it_;
        --step_;
        return *this;
    }

    const_iterator
    operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    value_type
    operator*() const noexcept
    {
        BOOST_ASSERT(step_ < ord_);
        value_type v(*it_);
        if(step_ == ord_ - 1 && chop_)
            v = value_type(
                v.data(), chop_);
        if(step_ == 0)
            v += trim_;
        return v;
    }
};

//------------------------------------------------------------------------------

template<class BufferSequence>
buffers_subrange<BufferSequence>::
buffers_subrange(
    buffers_subrange const& other)
    : b_(other.b_)
    , begin_(std::next(
        net::buffer_sequence_begin(b_),
            other.d0_))
    , end_(std::next(begin_, other.ord_))
    , d0_(other.d0_)
    , trim_(other.trim_)
    , chop_(other.chop_)
    , ord_(other.ord_)
{
}

template<class BufferSequence>
auto
buffers_subrange<BufferSequence>::
operator=(buffers_subrange const& other) ->
    buffers_subrange&
{
    b_ = other.b_;
    begin_ = std::next(
        net::buffer_sequence_begin(b_),
            other.d0_);
    end_ = std::next(begin_, other.ord_);
    d0_ = other.d0_;
    trim_ = other.trim_;
    chop_ = other.chop_;
    ord_ = other.ord_;
    return *this;
}


template<class BufferSequence>
buffers_subrange<BufferSequence>::
buffers_subrange(
    BufferSequence const& buffers)
    : buffers_subrange(buffers, 0,
        (std::numeric_limits<std::size_t>::max)())
{
}

template<class BufferSequence>
buffers_subrange<BufferSequence>::
buffers_subrange(
    BufferSequence const& buffers,
    std::size_t n)
    : buffers_subrange(0, n)
{
}

template<class BufferSequence>
buffers_subrange<BufferSequence>::
buffers_subrange(
    BufferSequence const& buffers,
    std::size_t pos, std::size_t n)
    : b_(buffers)
{
    auto it = net::buffer_sequence_begin(b_);
    auto end = net::buffer_sequence_end(b_);
    if(it != end && n != 0)
    {
        for(;;)
        {
            net::const_buffer cb(*it);
            if(pos < cb.size())
            {
                // cb contains pos
                d0_ = ord_;
                begin_ = it++;
                trim_ = pos;
                ++ord_;

                if( n <= cb.size() - pos ||
                    it == end)
                {
                    // cb contains pos+n
                    end_ = it;
                    chop_ = std::min<std::size_t>(
                        pos + n, cb.size());
                    break;
                }

                for(;;)
                {
                    n -= cb.size() - pos;
                    cb = *it++;
                    ++ord_;
                    if( n <= cb.size() - pos ||
                        it == end)
                    {
                        // cb contains n
                        end_ = it;
                        chop_ = std::min<std::size_t>(
                            n, cb.size());
                        break;
                    }
                }
                break;
            }

            pos -= cb.size();
            if(++it == end)
            {
                begin_ =
                    net::buffer_sequence_begin(b_);
                end_ = begin_;
                d0_ = 0;
                trim_ = 0;
                chop_ = 0;
                ord_ = 0;
                break;
            }
        }
    }
    else
    {
        begin_ =
            net::buffer_sequence_begin(b_);
        end_ = begin_;
        d0_ = 0;
        trim_ = 0;
        chop_ = 0;
        ord_ = 0;
    }
    BOOST_ASSERT(
        ord_ != 0 || begin_ == end_);
    BOOST_ASSERT(
        ord_ != 1 || chop_ >= trim_);
    BOOST_ASSERT(ord_ >= d0_);
}

template<class BufferSequence>
auto
buffers_subrange<BufferSequence>::
begin() const noexcept ->
    const_iterator
{
    return const_iterator(this,
        begin_, trim_, chop_, 0, ord_);
}

template<class BufferSequence>
auto
buffers_subrange<BufferSequence>::
end() const noexcept ->
    const_iterator
{
    return const_iterator(this,
        end_, trim_, chop_, ord_, ord_);
}

template<class BufferSequence>
void
buffers_subrange<BufferSequence>::
consume(std::size_t n)
{
    if(n == 0 || ord_ == 0)
        return;
    net::const_buffer cb = *begin_;
    if(ord_ == 1)
    {
        BOOST_ASSERT(chop_ >= trim_);
        if(n < chop_ - trim_)
        {
            trim_ += n;
        }
        else
        {
            end_ = begin_;
            trim_ = 0;
            chop_ = 0;
            ord_ = 0;
        }
        return;
    }
    if(n < cb.size() - trim_)
    {
        trim_ += n;
        return;
    }

    n -= cb.size() - trim_;
    trim_ = 0;
    for(;;)
    {
        --ord_;
        ++d0_;
        cb = *++begin_;
        BOOST_ASSERT(ord_ != 0);
        if(ord_ == 1)
        {
            if(n < chop_)
            {
                trim_ = n;
            }
            else
            {
                end_ = begin_;
                trim_ = 0;
                chop_ = 0;
                ord_ = 0;
            }
            return;
        }
        if(n < cb.size())
        {
            trim_ = n;
            return;
        }
        n -= cb.size();
    }
}

template<class BufferSequence>
auto
make_buffers_subrange(
    BufferSequence const& buffers,
    std::size_t pos, std::size_t n) ->
        buffers_subrange<BufferSequence>
{
    return buffers_subrange<
        BufferSequence>(buffers, pos, n);
}

} // beast
} // boost

#endif
