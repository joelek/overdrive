#pragma once

#include "shared.h"

namespace overdrive {
namespace memory {
	using namespace shared;

	auto test(
		const void* pointer,
		size_t size,
		byte_t value
	) -> bool_t;
}
}
