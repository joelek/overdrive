#pragma once

#include <string>
#include "../type.h"

#define HEXDUMP(var) fprintf(stderr, "%s\n", overdrive::hex::create_hex_dump(reinterpret_cast<const uint8_t*>(&var), sizeof(var)).c_str())

namespace overdrive {
namespace hex {
	using namespace type;

	auto create_hex_dump(
		const byte_t* bytes,
		size_t size
	) -> std::string;
}
}
