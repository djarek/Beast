//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_STATIC_BUFFER_IPP
#define BOOST_BEAST_IMPL_STATIC_BUFFER_IPP

#include <boost/beast/core/static_buffer.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>

namespace boost {
namespace beast {

static_buffer_base::
static_buffer_base(
    void* p, std::size_t size) noexcept
    : begin_(static_cast<char*>(p))
    , capacity_(size)
{
}

void
static_buffer_base::
clear() noexcept
{
    in_off_ = 0;
    in_size_ = 0;
    out_size_ = 0;
}

#ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1

auto
static_buffer_base::
data() const noexcept ->
    const_buffers_type
{
    if(! detail::sum_exceeds(
            in_off_, in_size_, capacity_))
        return {
            net::const_buffer{
                begin_ + in_off_, in_size_},
            net::const_buffer{
                begin_, 0}};
    return {
        net::const_buffer{
            begin_ + in_off_, capacity_ - in_off_},
        net::const_buffer{
            begin_, in_size_ - (capacity_ - in_off_)}};
}

auto
static_buffer_base::
data() noexcept ->
    mutable_data_type
{
    if(! detail::sum_exceeds(
            in_off_, in_size_, capacity_))
        return {
            net::mutable_buffer{
                begin_ + in_off_, in_size_},
            net::mutable_buffer{
                begin_, 0}};
    return {
        net::mutable_buffer{
            begin_ + in_off_, capacity_ - in_off_},
        net::mutable_buffer{
            begin_, in_size_ - (capacity_ - in_off_)}};
}

auto
static_buffer_base::
prepare(std::size_t n) ->
    mutable_buffers_type
{
    if(n > capacity_ - in_size_)
        BOOST_THROW_EXCEPTION(std::length_error{
            "static_buffer limit"});
    out_size_ = n;
    auto out_off = in_off_ + in_size_;
    if(out_off >= capacity_)
        out_off -= capacity_;
    if(! detail::sum_exceeds(
            out_off, out_size_, capacity_))
        return {
            net::mutable_buffer{
                begin_ + out_off, out_size_},
            net::mutable_buffer{
                begin_, 0}};
    return {
        net::mutable_buffer{
            begin_ + out_off, capacity_ - out_off},
        net::mutable_buffer{
            begin_, out_size_ - (capacity_ - out_off)}};
}

void
static_buffer_base::
commit(std::size_t n) noexcept
{
    in_size_ += (std::min)(n, out_size_);
    out_size_ = 0;
}

#endif

auto
static_buffer_base::
data(std::size_t pos, std::size_t n) noexcept ->
    mutable_buffers_type
{
    net::mutable_buffer b1;
    net::mutable_buffer b2;
    if(pos >= size() || n == 0)
        return { b1, b2 };
    if(detail::sum_exceeds(pos, n, size()))
        n = size() - pos;
    if(in_off_ + pos >= capacity_)
        pos -= capacity_;
    b1 = { begin_ + in_off_ + pos, (std::min)(
        capacity_ - in_off_, n) };
    if(b1.size() < n)
        b2 = { begin_, n - b1.size() };
    return { b1, b2 };
}

auto
static_buffer_base::
data(std::size_t pos, std::size_t n) const noexcept ->
    const_buffers_type
{
    net::const_buffer b1;
    net::const_buffer b2;
    if(pos >= size() || n == 0)
        return { b1, b2 };
    if(detail::sum_exceeds(pos, n, size()))
        n = size() - pos;
    if(in_off_ + pos >= capacity_)
        pos -= capacity_;
    b1 = { begin_ + in_off_ + pos, (std::min)(
        capacity_ - in_off_, n) };
    if(b1.size() < n)
        b2 = { begin_, n - b1.size() };
    return { b1, b2 };
}

void
static_buffer_base::
grow(std::size_t n)
{
    if(n > capacity_ - in_size_)
        BOOST_THROW_EXCEPTION(std::length_error{
            "static_buffer limit"});
    in_size_ += n;
    out_size_ = 0;
}

void
static_buffer_base::
shrink(std::size_t n)
{
    if(n >= in_size_)
    {
        in_off_ = 0;
        in_size_ = 0;
    }
    else
    {
        in_size_ -= n;
    }
    out_size_ = 0;
}

void
static_buffer_base::
consume(std::size_t n) noexcept
{
    if(n < in_size_)
    {
        in_off_ = in_off_ + n;
        if(in_off_ > capacity_)
            in_off_ -= capacity_;
        in_size_ -= n;
    }
    else
    {
        // rewind the offset, so the next call to prepare
        // can have a longer contiguous segment. this helps
        // algorithms optimized for larger buffers.
        in_off_ = 0;
        in_size_ = 0;
    }
}

} // beast
} // boost

#endif
