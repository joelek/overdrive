#pragma once

#include <cstdint>

namespace type {
	typedef int8_t si08;
	typedef uint8_t ui08;
	typedef int16_t si16;
	typedef uint16_t ui16;
	typedef int32_t si32;
	typedef uint32_t ui32;
	typedef int64_t si64;
	typedef uint64_t ui64;
	typedef bool flag;
	typedef uint8_t byte;
	typedef float fp32;
	typedef double fp64;
	typedef char ch08;
	typedef char16_t ch16;
	typedef char32_t ch32;

	template <typename A> using pointer = A*;
	template <typename A> using reference = A&;
	template <typename A> using temporary = A&&;
	template <typename A> using constant = A const;
	template <unsigned B, typename A> using array = A[B];
	template <typename A, typename... B> using function = A(B...);
}
