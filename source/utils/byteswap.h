#pragma once

#include "../type.h"

namespace byteswap {
	using namespace type;

	auto byteswap16(
		ui16_t value
	) -> ui16_t;

	auto byteswap32(
		ui32_t value
	) -> ui32_t;
}
