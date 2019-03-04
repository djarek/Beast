//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

// Test that header file is self-contained.
#include <boost/beast/core/buffers_subrange.hpp>
#include <boost/beast/core/buffer_traits.hpp>

#include "test_buffer.hpp"

#include <boost/beast/_experimental/unit_test/suite.hpp>

namespace boost {
namespace beast {

class buffers_subrange_test : public unit_test::suite
{
public:
    template<class B>
    static
    void
    check(
        B const& b,
        std::size_t pos,
        std::size_t n,
        std::size_t answer)
    {
        BEAST_EXPECT(buffer_size(
            make_buffers_subrange(b, pos, n)) == answer);
    }

    void
    testBuffers()
    {
        char buf[60];
        std::array<net::const_buffer, 3> v;
        v[0] = { buf +  0, 10 };
        v[1] = { buf + 10, 20 };
        v[2] = { buf + 30, 30 };
        BEAST_EXPECT(buffer_size(v) == 60);
        check(v,  0,  0,  0);
        check(v,  1,  0,  0);
        check(v,  0,  1,  1);
        check(v,  1,  1,  1);
        check(v,  0,  9,  9);
        check(v,  0, 10, 10);
        check(v,  0, 11, 11);
        check(v,  0, 20, 20);
        check(v,  5, 20, 20);
        check(v, 15,  5,  5);
        check(v, 15, 15, 15);
        check(v, 35, 25, 25);
        check(v, 35, 10, 10);
        check(v,  0, 99, 60);
        check(v,  5, 99, 55);
        check(v, 10, 99, 50);
        check(v, 15, 99, 45);
        check(v, 30, 99, 30);
        check(v, 45, 99, 15);
        check(v, 59,  1,  1);
        check(v, 59,  2,  1);
        check(v, 59,  2,  1);
        check(v, 60,  1,  0);
        check(v, 70,  0,  0);
        check(v, 70,  1,  0);

        auto b = make_buffers_subrange(v, 0, 99);
        BEAST_EXPECT(buffer_size(b) == 60);
        b.consume(1);
        BEAST_EXPECT(buffer_size(b) == 59);
        b.consume(9);
        BEAST_EXPECT(buffer_size(b) == 50);
        b.consume(15);
        BEAST_EXPECT(buffer_size(b) == 35);
        b.consume(10);
        BEAST_EXPECT(buffer_size(b) == 25);
        b.consume(60);
        BEAST_EXPECT(buffer_size(b) == 0);
    }

    void
    run() override
    {
        testBuffers();
    }
};

BEAST_DEFINE_TESTSUITE(beast,core,buffers_subrange);

} // beast
} // boost
