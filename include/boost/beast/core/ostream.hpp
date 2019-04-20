//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_OSTREAM_HPP
#define BOOST_BEAST_OSTREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/buffer_traits.hpp>
#include <boost/beast/core/detail/ostream.hpp>
#include <type_traits>
#include <streambuf>
#include <utility>

#ifdef BOOST_BEAST_ALLOW_DEPRECATED
#include <boost/beast/core/make_printable.hpp>
#endif

namespace boost {
namespace beast {

/** Return an output stream that formats values into a <em>DynamicBuffer</em>.

    This function wraps the caller provided <em>DynamicBuffer</em> into
    a `std::ostream` derived class, to allow `operator<<` stream style
    formatting operations.

    @par Example
    @code
        ostream(buffer) << "Hello, world!" << std::endl;
    @endcode

    @note Calling members of the underlying buffer before the output
    stream is destroyed results in undefined behavior.

    @param buffer An object meeting the requirements of <em>DynamicBuffer</em>
    into which the formatted output will be placed.

    @return An object derived from `std::ostream` which redirects output
    The wrapped dynamic buffer is not modified, a copy is made instead.
    Ownership of the underlying memory is not transferred, the application
    is still responsible for managing its lifetime. The caller is
    responsible for ensuring the dynamic buffer is not destroyed for the
    lifetime of the output stream.
*/
template<class DynamicBuffer>
#ifdef BOOST_BEAST_DOXYGEN
__implementation_defined__
#else
auto
#endif
ostream(DynamicBuffer&& buffer)
#ifndef BOOST_BEAST_DOXYGEN
    -> detail::ostream_helper<
        decltype(detail::make_dynamic_buffer_adaptor(
            std::forward<DynamicBuffer>(buffer))),
        char, std::char_traits<char>,
            detail::basic_streambuf_movable::value>
#endif
{
#if 0
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
#endif
    auto b = detail::make_dynamic_buffer_adaptor(
        std::forward<DynamicBuffer>(buffer));
    return detail::ostream_helper<
        decltype(b), char, std::char_traits<char>,
        detail::basic_streambuf_movable::value>{b};
}

//------------------------------------------------------------------------------

#ifdef BOOST_BEAST_ALLOW_DEPRECATED
template<class T>
detail::make_printable_adaptor<T>
buffers(T const& t)
{
    return make_printable(t);
}
#else
template<class T>
void buffers(T const&)
{
    static_assert(sizeof(T) == 0,
        "The function buffers() is deprecated, use make_printable() instead, "
        "or define BOOST_BEAST_ALLOW_DEPRECATED to silence this error.");
}
#endif

} // beast
} // boost

#endif
