#include "byteswap.h"

#include "endian.h"

namespace overdrive {
namespace byteswap {
	auto byteswap16(
		ui16_t value
	) -> ui16_t {
		auto pointer = reinterpret_cast<byte_t*>(&value);
		return (pointer[0] << 8) | (pointer[1] << 0);
	}

	auto byteswap16_on_big_endian_systems(
		ui16_t value
	) -> ui16_t {
		return endian::is_system_big_endian() ? byteswap16(value) : value;
	}

	auto byteswap16_on_little_endian_systems(
		ui16_t value
	) -> ui16_t {
		return endian::is_system_little_endian() ? byteswap16(value) : value;
	}

	auto byteswap32(
		ui32_t value
	) -> ui32_t {
		auto pointer = reinterpret_cast<byte_t*>(&value);
		return (pointer[0] << 24) | (pointer[1] << 16) | (pointer[2] << 8) | (pointer[3] << 0);
	}

	auto byteswap32_on_big_endian_systems(
		ui32_t value
	) -> ui32_t {
		return endian::is_system_big_endian() ? byteswap32(value) : value;
	}

	auto byteswap32_on_little_endian_systems(
		ui32_t value
	) -> ui32_t {
		return endian::is_system_little_endian() ? byteswap32(value) : value;
	}
}
}
