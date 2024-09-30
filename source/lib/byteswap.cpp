#include "byteswap.h"

namespace overdrive {
namespace byteswap {
	auto byteswap16(
		ui16_t value
	) -> ui16_t {
		auto pointer = reinterpret_cast<byte_t*>(&value);
		return (pointer[0] << 8) | (pointer[1] << 0);
	}

	auto byteswap32(
		ui32_t value
	) -> ui32_t {
		auto pointer = reinterpret_cast<byte_t*>(&value);
		return (pointer[0] << 24) | (pointer[1] << 16) | (pointer[2] << 8) | (pointer[3] << 0);
	}
}
}
