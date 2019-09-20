// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdexcept>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <piranha/detail/limits.hpp>
#include <piranha/k_packing.hpp>
#include <piranha/math/p_degree.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>
#include <piranha/symbols.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace piranha;

TEST_CASE("polynomial_mul_simple_test_p_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 100, symbol_set{"x"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 100, symbol_set{"x", "y"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 2, symbol_set{"x"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 2, symbol_set{"x", "y"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, mppp::integer<1>{1}, symbol_set{"x"});
    REQUIRE(retval == -y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, mppp::integer<1>{1}, symbol_set{"x", "y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 0, symbol_set{"x"});
    REQUIRE(retval == -y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 0, symbol_set{"x", "y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, -1, symbol_set{"x"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, -1, symbol_set{"x", "y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 100, symbol_set{"x"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 100, symbol_set{"x", "y"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 3, symbol_set{"x"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 3, symbol_set{"x", "y"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, mppp::integer<1>{2}, symbol_set{"x"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, mppp::integer<1>{2},
                                              symbol_set{"x", "y", "z"});
    REQUIRE(retval == -z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 1, symbol_set{"x"});
    REQUIRE(retval == -x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 1, symbol_set{"x", "y", "z"});
    REQUIRE(retval == -y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 0, symbol_set{"z"});
    REQUIRE(retval == x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 0, symbol_set{"x", "y", "z"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, -1, symbol_set{"y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, -1, symbol_set{"x", "y", "z"});
    REQUIRE(retval.empty());
    retval.clear_terms();
}

TEST_CASE("polynomial_mul_simple_test_p_truncated_large")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z, t, u] = make_polynomials<poly_t>("x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    const auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    const auto tmp_g(g);

    for (int i = 1; i < 8; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }

    const auto cmp = f * g;

    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z", "t", "u"});

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 1000, symbol_set{"x", "y", "z", "t", "u"});
    REQUIRE(retval == cmp);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 80, symbol_set{"x"});
    REQUIRE(retval == cmp);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 40, symbol_set{"x", "t", "u"});
    REQUIRE(p_degree(retval, symbol_set{"x", "t", "u"}) == 40);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 5, symbol_set{"z", "y"});
    REQUIRE(p_degree(retval, symbol_set{"z", "y"}) == 5);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();
}

TEST_CASE("polynomial_mul_mt_hm_test_p_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 100, symbol_set{"x"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 100, symbol_set{"x", "y"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 2, symbol_set{"x"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 2, symbol_set{"x", "y"});
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, mppp::integer<1>{1}, symbol_set{"x"});
    REQUIRE(retval == -y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, mppp::integer<1>{1}, symbol_set{"x", "y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 0, symbol_set{"x"});
    REQUIRE(retval == -y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 0, symbol_set{"x", "y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, -1, symbol_set{"x"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, -1, symbol_set{"x", "y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 100, symbol_set{"x"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 100, symbol_set{"x", "y"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 3, symbol_set{"x"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 3, symbol_set{"x", "y"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, mppp::integer<1>{2}, symbol_set{"x"});
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, mppp::integer<1>{2},
                                             symbol_set{"x", "y", "z"});
    REQUIRE(retval == -z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 1, symbol_set{"x"});
    REQUIRE(retval == -x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 1, symbol_set{"x", "y", "z"});
    REQUIRE(retval == -y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 0, symbol_set{"z"});
    REQUIRE(retval == x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 0, symbol_set{"x", "y", "z"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, -1, symbol_set{"y"});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, -1, symbol_set{"x", "y", "z"});
    REQUIRE(retval.empty());
    retval.clear_terms();
}

TEST_CASE("polynomial_mul_mt_hm_test_p_truncated_large")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z, t, u] = make_polynomials<poly_t>("x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    const auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    const auto tmp_g(g);

    for (int i = 1; i < 8; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }

    const auto cmp = f * g;

    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z", "t", "u"});

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 1000, symbol_set{"x", "y", "z", "t", "u"});
    REQUIRE(retval == cmp);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 80, symbol_set{"x"});
    REQUIRE(retval == cmp);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 40, symbol_set{"x", "t", "u"});
    REQUIRE(p_degree(retval, symbol_set{"x", "t", "u"}) == 40);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 5, symbol_set{"z", "y"});
    REQUIRE(p_degree(retval, symbol_set{"z", "y"}) == 5);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();
    retval.set_n_segments(0);
}

TEST_CASE("polynomial_pow_test")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::rational<1>>;
    using poly2_t = polynomial<pm_t, double>;

    auto [x, y] = make_polynomials<poly_t>("x", "y");

    REQUIRE(piranha::pow(poly_t{}, 4).empty());
    REQUIRE(piranha::pow(poly_t{3}, 3) == 27);
    REQUIRE(piranha::pow(x, 3) == x * x * x);
    const auto x_inv = piranha::pow(x, -1);
    REQUIRE(piranha::pow(-2 * x, -3) == -x_inv * x_inv * x_inv / 8);
    REQUIRE(x_inv * x == 1);
    REQUIRE(piranha::pow(x + y, 2) == x * x + y * y + 2 * x * y);

    // Test large integral exponentiations and overflow.
    REQUIRE(piranha::pow(3 * x / 4, 100)
            == mppp::rational<1>{"515377520732011331036461129765621272702107522001/"
                                 "1606938044258990275541962092341162602522202993782792835301376"}
                   * piranha::pow(x, 50) * piranha::pow(x, 50));

    auto [a, b] = make_polynomials<poly2_t>("a", "b");

    PIRANHA_REQUIRES_THROWS_CONTAINS(piranha::pow(a * a, detail::limits_max<long long>), std::overflow_error, "");

    // Get the delta bit width corresponding to a vector size of 2.
    const auto nbits = detail::k_packing_size_to_bits<long long>(2u);

    PIRANHA_REQUIRES_THROWS_CONTAINS(piranha::pow(a * a * b * b, detail::k_packing_get_climits<long long>(nbits, 0)[0]),
                                     std::overflow_error, "");
    PIRANHA_REQUIRES_THROWS_CONTAINS(piranha::pow(a * a * b * b, detail::k_packing_get_climits<long long>(nbits, 0)[1]),
                                     std::overflow_error, "");
}
