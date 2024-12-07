#pragma once

#include <cstdio>
#include <cstdint>
#include <format>
#include <string>

namespace overdrive {
namespace shared {
	typedef int8_t si08_t;
	typedef uint8_t ui08_t;
	typedef int16_t si16_t;
	typedef uint16_t ui16_t;
	typedef int32_t si32_t;
	typedef uint32_t ui32_t;
	typedef int64_t si64_t;
	typedef uint64_t ui64_t;
	typedef bool bool_t;
	typedef uint8_t byte_t;
	typedef float fp32_t;
	typedef double fp64_t;
	typedef char ch08_t;
	typedef char16_t ch16_t;
	typedef char32_t ch32_t;
	typedef int si_t;
	typedef unsigned ui_t;

	template <typename A> using pointer = A*;
	template <typename A> using reference = A&;
	template <typename A> using temporary = A&&;
	template <typename A> using constant = A const;
	template <unsigned B, typename A> using array = A[B];
	template <typename A, typename... B> using function = A(B...);

	auto create_hex_dump(
		const byte_t* bytes,
		size_t size
	) -> std::string;
}
}

#define OVERDRIVE_HEXDUMP(var) std::fprintf(stderr, "%s\n", overdrive::shared::create_hex_dump(reinterpret_cast<const byte_t*>(&var), sizeof(var)).c_str()); std::fflush(stderr);

#define OVERDRIVE_LOG(...) std::fprintf(stderr, "%s\n", std::format(__VA_ARGS__).c_str()); std::fflush(stderr);

#ifndef OVERDRIVE_VERSION
	#define OVERDRIVE_VERSION "?.?.?"
#endif
