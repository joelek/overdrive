#pragma once

#include "shared.h"

namespace overdrive {
namespace byteswap {
	using namespace shared;

	auto byteswap16(
		ui16_t value
	) -> ui16_t;

	auto byteswap16_on_big_endian_systems(
		ui16_t value
	) -> ui16_t;

	auto byteswap16_on_little_endian_systems(
		ui16_t value
	) -> ui16_t;

	auto byteswap32(
		ui32_t value
	) -> ui32_t;

	auto byteswap32_on_big_endian_systems(
		ui32_t value
	) -> ui32_t;

	auto byteswap32_on_little_endian_systems(
		ui32_t value
	) -> ui32_t;
}
}
