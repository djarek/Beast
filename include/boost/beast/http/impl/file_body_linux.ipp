//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_FILE_BODY_LINUX_IPP
#define BOOST_BEAST_HTTP_IMPL_FILE_BODY_LINUX_IPP

#if BOOST_BEAST_USE_LINUX_FILE

#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <sys/sendfile.h>
#include <cerrno>

namespace boost {
namespace beast {
namespace http {

namespace detail {


class null_lambda
{
public:
    template<class ConstBufferSequence>
    void
    operator()(error_code&,
        ConstBufferSequence const&) const
    {
        BOOST_ASSERT(false);
    }
};
} //  namespace detail

template<class Protocol, bool isRequest, class Fields>
std::size_t
write_some(
    boost::asio::basic_stream_socket<Protocol>& sock,
    serializer<isRequest,
        basic_file_body<file_posix>, Fields>& sr,
    error_code& ec)
{
    if(! sr.is_header_done())
    {
        sr.split(true);
        auto const bytes_transferred =
            detail::write_some_impl(sock, sr, ec);
        if(ec)
            return bytes_transferred;
        return bytes_transferred;
    }
    if(sr.get().chunked())
    {
        auto const bytes_transferred =
            detail::write_some_impl(sock, sr, ec);
        if(ec)
            return bytes_transferred;
        return bytes_transferred;
    }
    auto& w = sr.reader_impl();
    w.body_.file_.seek(r.pos_, ec);
    if(ec)
        return 0;

    const auto in_fd = r.body.file.native_handle();
    const auto out_fd = sock.native_handle();
    std::size_t const count =
        static_cast<std::size_t>(
        (std::min<std::uint64_t>)(
            (std::min<std::uint64_t>)(w.body_.last_ - w.pos_, sr.limit()),
            (std::numeric_limits<std::size_t>::max)()));

    std::size_t bytes_written = 0;
    const auto& sys_category = boost::system::system_category();
    do
    {
        ec.assign(0, ec.category());
        BOOST_ASSERT(w.pos_ <= w.body_.last_);
        if(w.pos_ >= w.body_.last_)
        {
            sr.next(ec, detail::null_lambda{});
            BOOST_ASSERT(! ec);
            BOOST_ASSERT(sr.is_done());
        }

        const auto ret = ::sendfile(out_fd, in_fd, nullptr, count);
        if (ret < 0)
        {
            ec = {errno, sys_category};
        }
        else
        {
            bytes_written += ret;
            r.pos_ += ret;
        }
    } while (ec == boost::system::errc::interrupted);

    return bytes_written;
}

} //  namespace http
} //  namespace beast
} //  namespace boost

#endif // BOOST_BEAST_USE_LINUX_FILE
#endif // BOOST_BEAST_HTTP_IMPL_FILE_BODY_LINUX_IPP
