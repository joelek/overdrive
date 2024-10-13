#include "endian.h"

namespace overdrive {
namespace endian {
	using namespace shared;

	auto is_system_big_endian()
	-> bool_t {
		auto value = "\x76\x54\x32\x10";
		return *reinterpret_cast<const ui32_t*>(value) == 0x76543210;
	}

	auto is_system_little_endian()
	-> bool_t {
		auto value = "\x10\x32\x54\x76";
		return *reinterpret_cast<const ui32_t*>(value) == 0x76543210;
	}
}
}
