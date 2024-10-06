#pragma once

#include "shared.h"

namespace overdrive {
namespace byteswap {
	using namespace shared;

	auto byteswap16(
		ui16_t value
	) -> ui16_t;

	auto byteswap32(
		ui32_t value
	) -> ui32_t;
}
}
