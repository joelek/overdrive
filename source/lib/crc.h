#pragma once

#include "shared.h"

namespace overdrive {
namespace crc {
	using namespace shared;

	auto compute_crc16(
		byte_t* buffer,
		size_t size
	) -> ui16_t;
}
}
