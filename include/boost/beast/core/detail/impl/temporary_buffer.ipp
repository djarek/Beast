//
// Copyright (c) 2019 Damian Jarek(damian.jarek93@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_IMPL_TEMPORARY_BUFFER_IPP
#define BOOST_BEAST_DETAIL_IMPL_TEMPORARY_BUFFER_IPP

#include <boost/beast/core/detail/temporary_buffer.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/core/exchange.hpp>

#include <memory>
#include <cstring>

namespace boost {
namespace beast {
namespace detail {


void
temporary_buffer::append(string_view sv)
{
    reserve_additional(sv.size());
    unsafe_append(sv);
}

void
temporary_buffer::append(string_view sv1, string_view sv2)
{
    reserve_additional(sv1.size() + sv2.size());
    unsafe_append(sv1);
    unsafe_append(sv2);
}

void
temporary_buffer::unsafe_append(string_view sv)
{
    auto n = sv.size();
    std::memcpy(&data_[size_], sv.data(), n);
    size_ += n;
}

void
temporary_buffer::reserve_additional(std::size_t sv_size)
{
    if (capacity_ - size_ >= sv_size)
    {
        return;
    }

    auto constexpr limit = (std::numeric_limits<std::size_t>::max)();
    if (beast::detail::sum_exceeds(capacity_, capacity_, limit) ||
        beast::detail::sum_exceeds(size_, sv_size, limit))
    {
        BOOST_THROW_EXCEPTION(std::length_error{"temporary_buffer::append"});
    }

    auto const new_cap = (std::max)(size_ + sv_size, capacity_*2);
    char* const p = new char[new_cap];
    std::memcpy(p, data_, size_);
    deallocate(boost::exchange(data_, p));
    capacity_ = new_cap;
}
} // detail
} // beast
} // boost

#endif
