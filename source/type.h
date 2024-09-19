#pragma once

#include <cstdint>

namespace type {
	typedef int8_t si08_t;
	typedef uint8_t ui08_t;
	typedef int16_t si16_t;
	typedef uint16_t ui16_t;
	typedef int32_t si32_t;
	typedef uint32_t ui32_t;
	typedef int64_t si64_t;
	typedef uint64_t ui64_t;
	typedef bool flag_t;
	typedef uint8_t byte_t;
	typedef float fp32_t;
	typedef double fp64_t;
	typedef char ch08_t;
	typedef char16_t ch16_t;
	typedef char32_t ch32_t;

	template <typename A> using pointer = A*;
	template <typename A> using reference = A&;
	template <typename A> using temporary = A&&;
	template <typename A> using constant = A const;
	template <unsigned B, typename A> using array = A[B];
	template <typename A, typename... B> using function = A(B...);
}
