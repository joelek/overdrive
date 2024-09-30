#include "bcd.h"

#include "exceptions.h"

namespace overdrive {
namespace bcd {
	auto decode(
		ui08_t byte
	) -> ui08_t {
		auto hi = (byte & 0xF0) >> 4;
		if (hi > 9) {
			throw exceptions::InvalidValueException("digit", hi, 0, 9);
		}
		auto lo = (byte & 0x0F) >> 0;
		if (lo > 9) {
			throw exceptions::InvalidValueException("digit", lo, 0, 9);
		}
		auto decoded = (hi * 10) + lo;
		return decoded;
	}

	auto encode(
		ui08_t byte
	) -> ui08_t {
		if (byte > 99) {
			throw exceptions::InvalidValueException("digits", byte, 0, 99);
		}
		auto hi = (byte / 10);
		auto lo = (byte % 10);
		auto encoded = (hi << 4) | (lo << 0);
		return encoded;
	}

	auto decode_address(
		const cd::SectorAddress& address
	) -> cd::SectorAddress {
		auto m = decode(address.m);
		auto s = decode(address.s);
		auto f = decode(address.f);
		return { m, s, f };
	}

	auto encode_address(
		const cd::SectorAddress& address
	) -> cd::SectorAddress {
		auto m = encode(address.m);
		auto s = encode(address.s);
		auto f = encode(address.f);
		return { m, s, f };
	}
}
}
