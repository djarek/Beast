//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

// Test that header file is self-contained.
#include <boost/beast/core/multi_buffer.hpp>

#include "test_buffer.hpp"

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/core/read_size.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/test/test_allocator.hpp>
#include <boost/beast/_experimental/unit_test/suite.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <memory>
#include <string>

namespace boost {
namespace beast {

class multi_buffer_test : public beast::unit_test::suite
{
public:
    BOOST_STATIC_ASSERT(
        is_mutable_dynamic_buffer<multi_buffer>::value);

/*
#if ! BOOST_WORKAROUND(BOOST_LIBSTDCXX_VERSION, < 50000) && \
    ! BOOST_WORKAROUND(BOOST_MSVC, < 1910)
    BOOST_STATIC_ASSERT(std::is_trivially_copyable<
        multi_buffer::const_buffers_type>::value);
    BOOST_STATIC_ASSERT(std::is_trivially_copyable<
        multi_buffer::mutable_data_type>::value);
#endif
*/

    template<class Alloc1, class Alloc2>
    static
    bool
    eq( basic_multi_buffer<Alloc1> const& mb1,
        basic_multi_buffer<Alloc2> const& mb2)
    {
        return buffers_to_string(mb1.data()) ==
            buffers_to_string(mb2.data());
    }

    void
    testShrinkToFit()
    {
        // empty list

        {
            multi_buffer b;
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 0);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 0);
        }

        // zero readable bytes

        {
            multi_buffer b;
            b.prepare(512);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 0);
        }

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(32);
            b.consume(32);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 0);
        }

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(512);
            b.prepare(512);
            b.clear();
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 0);
        }

        // unused list

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(512);
            b.prepare(512);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 512);
            BEAST_EXPECT(b.capacity() == 512);
        }

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(512);
            b.prepare(512);
            b.prepare(1024);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 512);
            BEAST_EXPECT(b.capacity() == 512);
        }

        // partial last buffer

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(512);
            b.prepare(512);
            b.commit(88);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 600);
            BEAST_EXPECT(b.capacity() == 600);
        }

        // shrink front of first buffer

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(512);
            b.consume(12);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 500);
            BEAST_EXPECT(b.capacity() == 500);
        }

        // shrink ends of first buffer

        {
            multi_buffer b;
            b.prepare(512);
            b.commit(500);
            b.consume(100);
            b.shrink_to_fit();
            BEAST_EXPECT(b.size() == 400);
            BEAST_EXPECT(b.capacity() == 400);
        }
    }

    void
    testDynamicBuffer()
    {
        multi_buffer b(30);
        BEAST_EXPECT(b.max_size() == 30);
        test_dynamic_buffer_v1(b);
        test_mutable_dynamic_buffer_v1(b);
        test_dynamic_buffer_v2(b);
    }

    void
    testMembers()
    {
        using namespace test;

        // compare equal
        using equal_t = test::test_allocator<char,
            true, true, true, true, true>;

        // compare not equal
        using unequal_t = test::test_allocator<char,
            false, true, true, true, true>;

        // construction
        {
            {
                multi_buffer b;
                BEAST_EXPECT(b.capacity() == 0);
            }
            {
                multi_buffer b{500};
                BEAST_EXPECT(b.capacity() == 0);
                BEAST_EXPECT(b.max_size() == 500);
            }
            {
                unequal_t a1;
                basic_multi_buffer<unequal_t> b{a1};
                BEAST_EXPECT(b.get_allocator() == a1);
                BEAST_EXPECT(b.get_allocator() != unequal_t{});
            }
            {
                unequal_t a1;
                basic_multi_buffer<unequal_t> b{500, a1};
                BEAST_EXPECT(b.capacity() == 0);
                BEAST_EXPECT(b.max_size() == 500);
                BEAST_EXPECT(b.get_allocator() == a1);
                BEAST_EXPECT(b.get_allocator() != unequal_t{});
            }
        }

        // move construction
        {
            {
                basic_multi_buffer<equal_t> b1{30};
                BEAST_EXPECT(b1.get_allocator()->nmove == 0);
                ostream(b1) << "Hello";
                basic_multi_buffer<equal_t> b2{std::move(b1)};
                BEAST_EXPECT(b2.get_allocator()->nmove == 1);
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(b1.capacity() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
                BEAST_EXPECT(b1.max_size() == b2.max_size());
            }
            // allocators equal
            {
                basic_multi_buffer<equal_t> b1{30};
                ostream(b1) << "Hello";
                equal_t a;
                basic_multi_buffer<equal_t> b2{std::move(b1), a};
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(b1.capacity() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
                BEAST_EXPECT(b1.max_size() == b2.max_size());
            }
            {
                // allocators unequal
                basic_multi_buffer<unequal_t> b1{30};
                ostream(b1) << "Hello";
                unequal_t a;
                basic_multi_buffer<unequal_t> b2{std::move(b1), a};
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(b1.capacity() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
                BEAST_EXPECT(b1.max_size() == b2.max_size());
            }
        }

        // copy construction
        {
            {
                basic_multi_buffer<equal_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<equal_t> b2{b1};
                BEAST_EXPECT(b1.get_allocator() == b2.get_allocator());
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                basic_multi_buffer<unequal_t> b1;
                ostream(b1) << "Hello";
                unequal_t a;
                basic_multi_buffer<unequal_t> b2(b1, a);
                BEAST_EXPECT(b1.get_allocator() != b2.get_allocator());
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                basic_multi_buffer<equal_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<unequal_t> b2(b1);
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                basic_multi_buffer<unequal_t> b1;
                ostream(b1) << "Hello";
                equal_t a;
                basic_multi_buffer<equal_t> b2(b1, a);
                BEAST_EXPECT(b2.get_allocator() == a);
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
        }

        // move assignment
        {
            {
                multi_buffer b1;
                ostream(b1) << "Hello";
                multi_buffer b2;
                b2 = std::move(b1);
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(b1.capacity() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                using na_t = test::test_allocator<char,
                    false, true, false, true, true>;
                basic_multi_buffer<na_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<na_t> b2;
                b2 = std::move(b1);
                BEAST_EXPECT(b1.get_allocator() != b2.get_allocator());
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(b1.capacity() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                // propagate_on_container_move_assignment : true
                using pocma_t = test::test_allocator<char,
                    true, true, true, true, true>;
                basic_multi_buffer<pocma_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<pocma_t> b2;
                b2 = std::move(b1);
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                // propagate_on_container_move_assignment : false
                using pocma_t = test::test_allocator<char,
                    true, true, false, true, true>;
                basic_multi_buffer<pocma_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<pocma_t> b2;
                b2 = std::move(b1);
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
        }

        // copy assignment
        {
            {
                multi_buffer b1;
                ostream(b1) << "Hello";
                multi_buffer b2;
                b2 = b1;
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
                basic_multi_buffer<equal_t> b3;
                b3 = b2;
                BEAST_EXPECT(buffers_to_string(b3.data()) == "Hello");
            }
            {
                // propagate_on_container_copy_assignment : true
                using pocca_t = test::test_allocator<char,
                    true, true, true, true, true>;
                basic_multi_buffer<pocca_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<pocca_t> b2;
                b2 = b1;
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
            {
                // propagate_on_container_copy_assignment : false
                using pocca_t = test::test_allocator<char,
                    true, false, true, true, true>;
                basic_multi_buffer<pocca_t> b1;
                ostream(b1) << "Hello";
                basic_multi_buffer<pocca_t> b2;
                b2 = b1;
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
            }
        }

        // max_size
        {
            multi_buffer b{10};
            BEAST_EXPECT(b.max_size() == 10);
            b.max_size(32);
            BEAST_EXPECT(b.max_size() == 32);
        }

        // allocator max_size
        {
            basic_multi_buffer<equal_t> b;
            auto a = b.get_allocator();
            BOOST_STATIC_ASSERT(
                ! std::is_const<decltype(a)>::value);
            a->max_size = 30;
            try
            {
                b.prepare(1000);
                fail("", __FILE__, __LINE__);
            }
            catch(std::length_error const&)
            {
                pass();
            }
            try
            {
                b.reserve(1000);
                fail("", __FILE__, __LINE__);
            }
            catch(std::length_error const&)
            {
                pass();
            }
        }


        // prepare
        {
            {
                multi_buffer b{100};
                try
                {
                    b.prepare(b.max_size() + 1);
                    fail("", __FILE__, __LINE__);
                }
                catch(std::length_error const&)
                {
                    pass();
                }
            }
            {
                string_view const s = "Hello, world!";
                multi_buffer b1{64};
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(b1.max_size() == 64);
                BEAST_EXPECT(b1.capacity() == 0);
                ostream(b1) << s;
                BEAST_EXPECT(buffers_to_string(b1.data()) == s);
                {
                    multi_buffer b2{b1};
                    BEAST_EXPECT(buffers_to_string(b2.data()) == s);
                    b2.consume(7);
                    BEAST_EXPECT(buffers_to_string(b2.data()) == s.substr(7));
                }
                {
                    multi_buffer b2{64};
                    b2 = b1;
                    BEAST_EXPECT(buffers_to_string(b2.data()) == s);
                    b2.consume(7);
                    BEAST_EXPECT(buffers_to_string(b2.data()) == s.substr(7));
                }
            }
            {
                multi_buffer b;
                b.prepare(1000);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.commit(1);
                BEAST_EXPECT(b.size() == 1);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.prepare(1000);
                BEAST_EXPECT(b.size() == 1);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.prepare(1500);
                BEAST_EXPECT(b.capacity() >= 1000);
            }
            {
                multi_buffer b;
                b.prepare(1000);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.commit(1);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.prepare(1000);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.prepare(2000);
                BEAST_EXPECT(b.capacity() >= 2000);
                b.commit(2);
            }
            {
                multi_buffer b;
                b.prepare(1000);
                BEAST_EXPECT(b.capacity() >= 1000);
                b.prepare(2000);
                BEAST_EXPECT(b.capacity() >= 2000);
                b.prepare(4000);
                BEAST_EXPECT(b.capacity() >= 4000);
                b.prepare(50);
                BEAST_EXPECT(b.capacity() >= 50);
            }
        }

        // commit
        {
            {
                multi_buffer b;
                b.prepare(16);
                b.commit(16);
                auto const n =
                    b.capacity() - b.size();
                b.prepare(n);
                b.commit(n);
                auto const size =
                    b.size();
                auto const capacity =
                    b.capacity();
                b.commit(1);
                BEAST_EXPECT(b.size() == size);
                BEAST_EXPECT(b.capacity() == capacity);
            }

            multi_buffer b;
            b.prepare(1000);
            BEAST_EXPECT(b.capacity() >= 1000);
            b.commit(1000);
            BEAST_EXPECT(b.size() == 1000);
            BEAST_EXPECT(b.capacity() >= 1000);
            b.consume(1000);
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 0);
            b.prepare(1000);
            b.commit(650);
            BEAST_EXPECT(b.size() == 650);
            BEAST_EXPECT(b.capacity() >= 1000);
            b.prepare(1000);
            BEAST_EXPECT(b.capacity() >= 1650);
            b.commit(100);
            BEAST_EXPECT(b.size() == 750);
            BEAST_EXPECT(b.capacity() >= 1000);
            b.prepare(1000);
            BEAST_EXPECT(b.capacity() >= 2000);
            b.commit(500);
        }

        // consume
        {
            multi_buffer b;
            b.prepare(1000);
            BEAST_EXPECT(b.capacity() >= 1000);
            b.commit(1000);
            BEAST_EXPECT(b.size() == 1000);
            BEAST_EXPECT(b.capacity() >= 1000);
            b.prepare(1000);
            BEAST_EXPECT(b.capacity() >= 2000);
            b.commit(750);
            BEAST_EXPECT(b.size() == 1750);
            b.consume(500);
            BEAST_EXPECT(b.size() == 1250);
            b.consume(500);
            BEAST_EXPECT(b.size() == 750);
            b.prepare(250);
            b.consume(750);
            BEAST_EXPECT(b.size() == 0);
            b.prepare(1000);
            b.commit(800);
            BEAST_EXPECT(b.size() == 800);
            b.prepare(1000);
            b.commit(600);
            BEAST_EXPECT(b.size() == 1400);
            b.consume(1400);
            BEAST_EXPECT(b.size() == 0);
        }

        // reserve
        {
            multi_buffer b;
            BEAST_EXPECT(b.capacity() == 0);
            b.reserve(50);
            BEAST_EXPECT(b.capacity() >= 50);
            b.prepare(20);
            b.commit(20);
            b.reserve(50);
            BEAST_EXPECT(b.capacity() >= 50);
            BEAST_EXPECT(b.size() > 1);
            auto capacity = b.capacity();
            b.reserve(b.size() - 1);
            BEAST_EXPECT(b.capacity() == capacity);
            b.reserve(b.capacity() + 1);
            BEAST_EXPECT(b.capacity() > capacity);
            capacity = b.capacity();
            BEAST_EXPECT(buffers_length(
                b.prepare(b.capacity() + 200)) > 1);
            BEAST_EXPECT(b.capacity() > capacity);
            b.reserve(b.capacity() + 2);
            BEAST_EXPECT(b.capacity() > capacity);
            capacity = b.capacity();
            b.reserve(b.capacity());
            BEAST_EXPECT(b.capacity() == capacity);
        }

        // shrink to fit
        {
            {
                multi_buffer b;
                BEAST_EXPECT(b.capacity() == 0);
                b.prepare(50);
                BEAST_EXPECT(b.capacity() >= 50);
                b.commit(50);
                BEAST_EXPECT(b.capacity() >= 50);
                b.prepare(75);
                BEAST_EXPECT(b.capacity() >= 125);
                b.shrink_to_fit();
                BEAST_EXPECT(b.capacity() >= b.size());
            }
            {
                multi_buffer b;
                b.prepare(2000);
                BEAST_EXPECT(b.capacity() == 2000);
                b.commit(1800);
                BEAST_EXPECT(b.size() == 1800);
                BEAST_EXPECT(b.capacity() == 2000);
                b.prepare(5000);
                BEAST_EXPECT(b.capacity() == 6800);
                b.shrink_to_fit();
                BEAST_EXPECT(b.capacity() == 2000);
            }
        }

        // clear
        {
            multi_buffer b;
            BEAST_EXPECT(b.capacity() == 0);
            b.prepare(50);
            b.commit(50);
            BEAST_EXPECT(b.size() == 50);
            BEAST_EXPECT(b.capacity() == 512);
            b.clear();
            BEAST_EXPECT(b.size() == 0);
            BEAST_EXPECT(b.capacity() == 512);
        }

        // swap
        {
            {
                // propagate_on_container_swap : true
                using pocs_t = test::test_allocator<char,
                    false, true, true, true, true>;
                pocs_t a1, a2;
                BEAST_EXPECT(a1 != a2);
                basic_multi_buffer<pocs_t> b1{a1};
                ostream(b1) << "Hello";
                basic_multi_buffer<pocs_t> b2{a2};
                BEAST_EXPECT(b1.get_allocator() == a1);
                BEAST_EXPECT(b2.get_allocator() == a2);
                swap(b1, b2);
                BEAST_EXPECT(b1.get_allocator() == a2);
                BEAST_EXPECT(b2.get_allocator() == a1);
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
                swap(b1, b2);
                BEAST_EXPECT(b1.get_allocator() == a1);
                BEAST_EXPECT(b2.get_allocator() == a2);
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(b2.size() == 0);
            }
            {
                // propagate_on_container_swap : false
                using pocs_t = test::test_allocator<char,
                    true, true, true, false, true>;
                pocs_t a1, a2;
                BEAST_EXPECT(a1 == a2);
                BEAST_EXPECT(a1.id() != a2.id());
                basic_multi_buffer<pocs_t> b1{a1};
                ostream(b1) << "Hello";
                basic_multi_buffer<pocs_t> b2{a2};
                BEAST_EXPECT(b1.get_allocator() == a1);
                BEAST_EXPECT(b2.get_allocator() == a2);
                swap(b1, b2);
                BEAST_EXPECT(b1.get_allocator().id() == a1.id());
                BEAST_EXPECT(b2.get_allocator().id() == a2.id());
                BEAST_EXPECT(b1.size() == 0);
                BEAST_EXPECT(buffers_to_string(b2.data()) == "Hello");
                swap(b1, b2);
                BEAST_EXPECT(b1.get_allocator().id() == a1.id());
                BEAST_EXPECT(b2.get_allocator().id() == a2.id());
                BEAST_EXPECT(buffers_to_string(b1.data()) == "Hello");
                BEAST_EXPECT(b2.size() == 0);
            }
        }

        // read_size
        {
            multi_buffer b{10};
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
    }

    void
    testMatrix1()
    {
        string_view s = "Hello, world";
        BEAST_EXPECT(s.size() == 12);
        for(std::size_t i = 1; i < 12; ++i) {
        for(std::size_t x = 1; x < 4; ++x) {
        for(std::size_t y = 1; y < 4; ++y) {
        std::size_t z = s.size() - (x + y);
        {
            multi_buffer b;
            b.commit(net::buffer_copy(
                b.prepare(x), net::buffer(s.data(), x)));
            b.commit(net::buffer_copy(
                b.prepare(y), net::buffer(s.data()+x, y)));
            b.commit(net::buffer_copy(
                b.prepare(z), net::buffer(s.data()+x+y, z)));
            BEAST_EXPECT(buffers_to_string(b.data()) == s);
            {
                multi_buffer mb2{b};
                BEAST_EXPECT(eq(b, mb2));
            }
            {
                multi_buffer mb2;
                mb2 = b;
                BEAST_EXPECT(eq(b, mb2));
            }
            {
                multi_buffer mb2{std::move(b)};
                BEAST_EXPECT(buffers_to_string(mb2.data()) == s);
                BEAST_EXPECT(b.size() == 0);
                BEAST_EXPECT(buffer_bytes(b.data()) == 0);
                b = std::move(mb2);
                BEAST_EXPECT(buffers_to_string(b.data()) == s);
                BEAST_EXPECT(mb2.size() == 0);
                BEAST_EXPECT(buffer_bytes(mb2.data()) == 0);
            }
        }
        }}}
    }

    void
    testMatrix2()
    {
        using namespace test;
        std::string const s = "Hello, world";
        BEAST_EXPECT(s.size() == 12);
        for(std::size_t i = 1; i < 12; ++i) {
        for(std::size_t x = 1; x < 4; ++x) {
        for(std::size_t y = 1; y < 4; ++y) {
        for(std::size_t t = 1; t < 4; ++ t) {
        for(std::size_t u = 1; u < 4; ++ u) {
        std::size_t z = s.size() - (x + y);
        std::size_t v = s.size() - (t + u);
        {
            multi_buffer b;
            {
                auto d = b.prepare(z);
                BEAST_EXPECT(buffer_bytes(d) == z);
            }
            {
                auto d = b.prepare(0);
                BEAST_EXPECT(buffer_bytes(d) == 0);
            }
            {
                auto d = b.prepare(y);
                BEAST_EXPECT(buffer_bytes(d) == y);
            }
            {
                auto d = b.prepare(x);
                BEAST_EXPECT(buffer_bytes(d) == x);
                b.commit(buffer_copy(d, net::buffer(s.data(), x)));
            }
            BEAST_EXPECT(b.size() == x);
            BEAST_EXPECT(buffer_bytes(b.data()) == b.size());
            {
                auto d = b.prepare(x);
                BEAST_EXPECT(buffer_bytes(d) == x);
            }
            {
                auto d = b.prepare(0);
                BEAST_EXPECT(buffer_bytes(d) == 0);
            }
            {
                auto d = b.prepare(z);
                BEAST_EXPECT(buffer_bytes(d) == z);
            }
            {
                auto d = b.prepare(y);
                BEAST_EXPECT(buffer_bytes(d) == y);
                b.commit(buffer_copy(d, net::buffer(s.data()+x, y)));
            }
            b.commit(1);
            BEAST_EXPECT(b.size() == x + y);
            BEAST_EXPECT(buffer_bytes(b.data()) == b.size());
            {
                auto d = b.prepare(x);
                BEAST_EXPECT(buffer_bytes(d) == x);
            }
            {
                auto d = b.prepare(y);
                BEAST_EXPECT(buffer_bytes(d) == y);
            }
            {
                auto d = b.prepare(0);
                BEAST_EXPECT(buffer_bytes(d) == 0);
            }
            {
                auto d = b.prepare(z);
                BEAST_EXPECT(buffer_bytes(d) == z);
                b.commit(buffer_copy(d, net::buffer(s.data()+x+y, z)));
            }
            b.commit(2);
            BEAST_EXPECT(b.size() == x + y + z);
            BEAST_EXPECT(buffer_bytes(b.data()) == b.size());
            BEAST_EXPECT(buffers_to_string(b.data()) == s);
            b.consume(t);
            {
                auto d = b.prepare(0);
                BEAST_EXPECT(buffer_bytes(d) == 0);
            }
            BEAST_EXPECT(buffers_to_string(b.data()) ==
                s.substr(t, std::string::npos));
            b.consume(u);
            BEAST_EXPECT(buffers_to_string(b.data()) ==
                s.substr(t + u, std::string::npos));
            b.consume(v);
            BEAST_EXPECT(buffers_to_string(b.data()).empty());
            b.consume(1);
            {
                auto d = b.prepare(0);
                BEAST_EXPECT(buffer_bytes(d) == 0);
            }
        }
        }}}}}
    }

    void
    testMultiBuffer()
    {
        auto const check =
            [](
                multi_buffer::const_buffers_type b,
                std::size_t n, long len)
            {
                BEAST_EXPECT(
                    net::buffer_size(b) == n);
                BEAST_EXPECT(
                    buffer_bytes(b) == n);
                BEAST_EXPECT(std::distance(
                    b.begin(), b.end()) == len);
            };

        // length <= 1
        {
            multi_buffer b;

            check(b.data(0, 0), 0, 0);
            check(b.data(0, 1), 0, 0);
            check(b.data(1, 0), 0, 0);
            check(b.data(1, 1), 0, 0);

            b.grow(99);
            BEAST_EXPECT(b.size() == 99);
            BEAST_EXPECT(b.capacity() == 512);

            check(b.data(  0,    0),  0,   0);
            check(b.data(  0,    1),  1,   1);
            check(b.data(  1,    0),  0,   0);
            check(b.data(  1,    1),  1,   1);

            check(b.data(  0,   50), 50,   1);
            check(b.data(  0,   99), 99,   1);
            check(b.data(  0,  100), 99,   1);

            check(b.data( 50,  25),  25,   1);
            check(b.data( 50,  50),  49,   1);

            b.shrink(20);
            BEAST_EXPECT(b.size() == 79);
            BEAST_EXPECT(b.capacity() == 512);

            check(b.data(  0,    0),  0,   0);
            check(b.data(  0,    1),  1,   1);
            check(b.data(  1,    0),  0,   0);
            check(b.data(  1,    1),  1,   1);

            check(b.data(  0,   50), 50,   1);
            check(b.data(  0,   99), 79,   1);
            check(b.data(  0,  100), 79,   1);

            check(b.data( 50,  25),  25,   1);
            check(b.data( 50,  50),  29,   1);

            b.grow(433);
            BEAST_EXPECT(b.size() == 512);
            BEAST_EXPECT(b.capacity() == 512);

            check(b.data(  0,    0),   0,   0);
            check(b.data(  0,    1),   1,   1);
            check(b.data(  1,    0),   0,   0);
            check(b.data(  1,    1),   1,   1);

            check(b.data(  0,  200), 200,   1);
            check(b.data(  0,  512), 512,   1);
            check(b.data(  0,  600), 512,   1);

            check(b.data(200,  250), 250,   1);
            check(b.data(200,  315), 312,   1);

            b.consume(12);

            check(b.data(  0,    0),   0,   0);
            check(b.data(  0,    1),   1,   1);
            check(b.data(  1,    0),   0,   0);
            check(b.data(  1,    1),   1,   1);

            check(b.data(  0,  200), 200,   1);
            check(b.data(  0,  500), 500,   1);
            check(b.data(  0,  512), 500,   1);
            check(b.data(  0,  600), 500,   1);

            check(b.data(200,  250), 250,   1);
            check(b.data(200,  315), 300,   1);

            b.shrink(9999);
            BEAST_EXPECT(b.capacity() > 0);
            BEAST_EXPECT(b.size() == 0);
            check(b.data(0, 9999), 0, 0);
        }

        // length <= 2
        {
            multi_buffer b;

            b.grow(512);
            BEAST_EXPECT(b.size() == 512);
            BEAST_EXPECT(b.capacity() == 512);

            b.grow(487);
            BEAST_EXPECT(b.size() == 999);
            BEAST_EXPECT(b.capacity() == 1024);

            check(b.data(  0, 500), 500, 1);
            check(b.data(  0, 512), 512, 1);
            check(b.data(  0, 513), 513, 2);
            check(b.data(  0, 999), 999, 2);
            check(b.data(500, 200), 200, 2);
            check(b.data(500, 999), 499, 2);
            check(b.data(600, 100), 100, 1);
            check(b.data(600, 999), 399, 1);

            b.shrink(499);
            check(b.data(  0, 500), 500, 1);
            check(b.data(  0, 512), 500, 1);
            check(b.data(  0, 513), 500, 1);
            check(b.data(  0,2000), 500, 1);

            b.grow(499);
            b.consume(100);
            BEAST_EXPECT(b.size() == 899);
            BEAST_EXPECT(b.capacity() == 924);

            check(b.data(  0, 500), 500, 2);
            check(b.data(  0, 512), 512, 2);
            check(b.data(  0, 513), 513, 2);
            check(b.data(  0, 899), 899, 2);
            check(b.data(  0, 999), 899, 2);

            b.shrink(9999);
            BEAST_EXPECT(b.capacity() > 0);
            BEAST_EXPECT(b.size() == 0);
            check(b.data(0, 9999), 0, 0);
        }

        // length <= 3
        {
            multi_buffer b;
            b.grow(512);
            b.grow(512);
            b.grow(188);
            BEAST_EXPECT(b.capacity() == 2048);
            check(b.data(0, 1212), 1212, 3);
            b.consume(12);
            check(b.data(0, 1200), 1200, 3);

            check(b.data(   0,  250),  250, 1);
            check(b.data(   0,  500),  500, 1);
            check(b.data(   0, 1000), 1000, 2);
            check(b.data(   0, 1200), 1200, 3);
            check(b.data(   0, 2000), 1200, 3);

            check(b.data( 250,  500),  500, 2);
            check(b.data( 250, 1000),  950, 3);
            check(b.data( 500,  500),  500, 1);
            check(b.data( 500, 1000),  700, 2);

            check(b.data( 600,  100),  100, 1);
            check(b.data( 600,  412),  412, 1);
            check(b.data( 600,  512),  512, 2);

            b.shrink(9999);
            BEAST_EXPECT(b.capacity() > 0);
            BEAST_EXPECT(b.size() == 0);
            check(b.data(0, 9999), 0, 0);
        }
    }

    void
    run() override
    {
        testShrinkToFit();
        testDynamicBuffer();
        testMembers();
        testMatrix1();
        testMatrix2();
        testMultiBuffer();
    }
};

BEAST_DEFINE_TESTSUITE(beast,core,multi_buffer);

} // beast
} // boost
