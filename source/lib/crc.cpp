#include "crc.h"

namespace overdrive {
namespace crc {
	// This is the truncated version of P(X) = X^16 + X^12 + X^5 + 1.
	const auto TRUNCATED_16BIT_POLYNOMIAL = ui16_t(0x1021);

	namespace internal {
		auto CRC16_TABLE() -> constant<array<256, ui16_t>>& {
			static auto initialized = false;
			static ui16_t table[256];
			if (!initialized) {
				for (auto byte_index = size_t(0); byte_index < size_t(256); byte_index += 1) {
					auto value = ui16_t(byte_index << 8);
					for (auto bit_index = size_t(0); bit_index < size_t(8); bit_index += 1) {
						auto wont_overflow = (value & 0x8000) == 0;
						if (wont_overflow) {
							value = (value << 1);
						} else {
							value = (value << 1) ^ TRUNCATED_16BIT_POLYNOMIAL;
						}
					}
					table[byte_index] = value;
				}
				initialized = true;
			}
			return table;
		}
	}

	auto compute_crc16(
		byte_t* buffer,
		size_t size
	) -> ui16_t {
		auto crc = ui16_t(0);
		auto& table = internal::CRC16_TABLE();
		for (auto byte_index = size_t(0); byte_index < size; byte_index += 1) {
			auto byte = buffer[byte_index];
			crc = (crc << 8) ^ table[((crc >> 8) & 0xFF) ^ byte];
		}
		return ~crc;
	}
}
}
