//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

// Test that header file is self-contained.
#include <boost/beast/core/static_buffer.hpp>

#include "test_buffer.hpp"

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/core/read_size.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/_experimental/unit_test/suite.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <algorithm>
#include <cctype>
#include <string>

namespace boost {
namespace beast {

class static_buffer_test : public beast::unit_test::suite
{
public:
    BOOST_STATIC_ASSERT(
        is_mutable_dynamic_storage<
            static_buffer<13>>::value);

    BOOST_STATIC_ASSERT(
        is_mutable_dynamic_storage<
            static_buffer_base>::value);

    void
    testDynamicBuffer()
    {
        {
            static_buffer<100> b;

    #ifndef BOOST_ASIO_NO_DYNAMIC_BUFFER_V1

            BOOST_STATIC_ASSERT(
                net::is_dynamic_buffer_v1<
                    decltype(b.dynamic_buffer())>::value);

            BOOST_STATIC_ASSERT(
                net::is_dynamic_buffer_v1<
                    decltype(b.dynamic_buffer(10))>::value);

            BOOST_STATIC_ASSERT(
                net::is_dynamic_buffer_v1<decltype(
                    b.operator->())>::value);

    #endif

            BOOST_STATIC_ASSERT(
                net::is_dynamic_buffer_v2<
                    decltype(b.dynamic_buffer())>::value);

            BOOST_STATIC_ASSERT(
                net::is_dynamic_buffer_v2<
                    decltype(b.dynamic_buffer(10))>::value);

            BOOST_STATIC_ASSERT(
                net::is_dynamic_buffer_v2<decltype(
                    b.operator->())>::value);

            b->grow(0);
        }
        {
            static_buffer<13> b;
            test_dynamic_storage_v1(b);
            test_dynamic_storage_v2(b);
        }
        {
            static_buffer<100> b;
            test_dynamic_buffer_v1(b.dynamic_buffer(30));
            test_dynamic_buffer_v2(b.dynamic_buffer(120));
        }
    }

    void
    testMembers()
    {
        string_view const s = "Hello, world!";
        
        // static_buffer_base
        {
            char buf[64];
            static_buffer_base b{buf, sizeof(buf)};
            ostream(b.dynamic_buffer()) << s;
            BEAST_EXPECT(buffers_to_string(b.data()) == s);
            b.clear();
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(buffer_bytes(b.data()) == 0);
        }

        // static_buffer
        {
            static_buffer<64> b1;
            BEAST_EXPECT(b1.size() == 0);
            BEAST_EXPECT(b1.max_size() == 64);
            BEAST_EXPECT(b1.capacity() == 64);
            ostream(b1.dynamic_buffer()) << s;
            BEAST_EXPECT(buffers_to_string(b1.data()) == s);
            {
                static_buffer<64> b2{b1};
                BEAST_EXPECT(buffers_to_string(b2.data()) == s);
                b2.consume(7);
                BEAST_EXPECT(buffers_to_string(b2.data()) == s.substr(7));
            }
            {
                static_buffer<64> b2;
                b2 = b1;
                BEAST_EXPECT(buffers_to_string(b2.data()) == s);
                b2.consume(7);
                BEAST_EXPECT(buffers_to_string(b2.data()) == s.substr(7));
            }
        }

        // cause memmove
        {
            static_buffer<10> b;
            ostream(b.dynamic_buffer()) << "12345";
            b.consume(3);
            ostream(b.dynamic_buffer()) << "67890123";
            BEAST_EXPECT(buffers_to_string(b.data()) == "4567890123");
            try
            {
                b.prepare(1);
                fail("", __FILE__, __LINE__);
            }
            catch(std::length_error const&)
            {
                pass();
            }
        }

        // read_size
        {
            static_buffer<10> b;
            BEAST_EXPECT(read_size(b, 512) == 10);
            b.prepare(4);
            b.commit(4);
            BEAST_EXPECT(read_size(b, 512) == 6);
            b.consume(2);
            BEAST_EXPECT(read_size(b, 512) == 8);
            b.prepare(8);
            b.commit(8);
            BEAST_EXPECT(read_size(b, 512) == 0);
        }

        // base
        {
            static_buffer<10> b;
            [&](static_buffer_base& base)
            {
                BEAST_EXPECT(base.max_size() == b.capacity());
            }
            (b.base());

            [&](static_buffer_base const& base)
            {
                BEAST_EXPECT(base.max_size() == b.capacity());
            }
            (b.base());
        }

        // This exercises the wrap-around cases
        // for the circular buffer representation
        {
            static_buffer<5> b;
            {
                auto const mb = b.prepare(5);
                BEAST_EXPECT(buffers_length(mb) == 1);
            }
            b.commit(4);
            BEAST_EXPECT(buffers_length(b.data()) == 1);
            BEAST_EXPECT(buffers_length(b.cdata()) == 1);
            b.consume(3);
            {
                auto const mb = b.prepare(3);
                BEAST_EXPECT(buffers_length(mb) == 2);
                auto it1 = mb.begin();
                auto it2 = std::next(it1);
                BEAST_EXPECT(
                    net::const_buffer(*it1).data() >
                    net::const_buffer(*it2).data());
            }
            b.commit(2);
            {
                auto const mb = b.data();
                auto it1 = mb.begin();
                auto it2 = std::next(it1);
                BEAST_EXPECT(
                    net::const_buffer(*it1).data() >
                    net::const_buffer(*it2).data());
            }
            {
                auto const cb = b.cdata();
                auto it1 = cb.begin();
                auto it2 = std::next(it1);
                BEAST_EXPECT(
                    net::const_buffer(*it1).data() >
                    net::const_buffer(*it2).data());
            }
        }
    }

    void
    run() override
    {
        testDynamicBuffer();
        testMembers();
    }
};

BEAST_DEFINE_TESTSUITE(beast,core,static_buffer);

} // beast
} // boost
