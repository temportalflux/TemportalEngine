#ifndef MATH_POW_HPP
#define MATH_POW_HPP

// Source: http://www.cplusplus.com/forum/general/118995/

template < typename T, T b, unsigned int e >
struct pow : std::integral_constant< decltype(b * 1), b * pow<T, b, e - 1>::value > {};

template < typename T, T b >
struct pow<T, b, 0> : std::integral_constant< decltype(b * 1), T(1) > {};

#endif