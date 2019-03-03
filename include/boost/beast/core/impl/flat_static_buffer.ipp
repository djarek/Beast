//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_FLAT_STATIC_BUFFER_IPP
#define BOOST_BEAST_IMPL_FLAT_STATIC_BUFFER_IPP

#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace boost {
namespace beast {

/*  Layout:

      begin_     in_          out_        last_      end_
        |<------->|<---------->|<---------->|<------->|
                  |  readable  |  writable  |
*/

void
flat_static_buffer_base::
clear() noexcept
{
    in_ = begin_;
    out_ = begin_;
    last_ = begin_;
}

#ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1

auto
flat_static_buffer_base::
prepare(std::size_t n) ->
    mutable_buffers_type
{
    auto const len = size();
    if(detail::sum_exceeds(len, n, max_size()))
        BOOST_THROW_EXCEPTION(std::length_error{
            "flat_static_buffer_base too big"});
    grow(n);
    out_ = in_ + len;
    BOOST_ASSERT(out_ <= end_);
    BOOST_ASSERT(last_ == out_ + n);
    BOOST_ASSERT(last_ <= end_);
    return {out_, n};
}

#endif

auto
flat_static_buffer_base::
data(std::size_t pos, std::size_t n) noexcept ->
    mutable_buffers_type
{
    auto const len = size();
    if(pos > len)
        return {};
    if(detail::sum_exceeds(pos, n, len))
        n = len - pos;
    return mutable_buffers_type{
        in_ + pos, n };
}

auto
flat_static_buffer_base::
data(std::size_t pos, std::size_t n) const noexcept ->
    const_buffers_type
{
    auto const len = size();
    if(pos > len)
        return {};
    if(detail::sum_exceeds(pos, n, len))
        n = len - pos;
    return const_buffers_type{
        in_ + pos, n };
}

void
flat_static_buffer_base::
grow(std::size_t n)
{
    auto const len = size();
    if(detail::sum_exceeds(len, n, max_size()))
        BOOST_THROW_EXCEPTION(std::length_error{
            "basic_flat_buffer too big"});
    if(n <= dist(out_, end_))
    {
        out_ = out_ + n;
        last_ = out_;
        return;
    }
    if(n > capacity() - len)
        BOOST_THROW_EXCEPTION(std::length_error{
            "basic_flat_buffer overflow"});
    if(len > 0)
        std::memmove(begin_, in_, len);
    in_ = begin_;
    out_ = in_ + len + n;
    last_ = out_;
}

void
flat_static_buffer_base::
shrink(std::size_t n)
{
    if(n >= capacity())
    {
        clear();
        return;
    }
    out_ = in_ + size() - n;
    last_ = out_;
}

void
flat_static_buffer_base::
consume(std::size_t n) noexcept
{
    if(n >= size())
    {
        in_ = begin_;
        out_ = begin_;
        last_ = begin_;
        return;
    }
    in_ += n;
}

void
flat_static_buffer_base::
reset(void* p, std::size_t n) noexcept
{
    begin_ = static_cast<char*>(p);
    in_ = begin_;
    out_ = begin_;
    last_ = begin_;
    end_ = begin_ + n;
}

} // beast
} // boost

#endif