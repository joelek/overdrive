#include "bcd.h"

#include "../lib/exceptions.h"

namespace utils {
namespace bcd {
	auto decode(
		ui08_t byte
	) -> ui08_t {
		auto hi = (byte & 0xF0) >> 4;
		if (hi > 9) {
			throw overdrive::exceptions::InvalidValueException("digit", hi, 0, 9);
		}
		auto lo = (byte & 0x0F) >> 0;
		if (lo > 9) {
			throw overdrive::exceptions::InvalidValueException("digit", lo, 0, 9);
		}
		auto decoded = (hi * 10) + lo;
		return decoded;
	}

	auto encode(
		ui08_t byte
	) -> ui08_t {
		if (byte > 99) {
			throw overdrive::exceptions::InvalidValueException("digits", byte, 0, 99);
		}
		auto hi = (byte / 10);
		auto lo = (byte % 10);
		auto encoded = (hi << 4) | (lo << 0);
		return encoded;
	}
}
}
