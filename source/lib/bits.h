#pragma once

#include <vector>
#include "shared.h"

namespace overdrive {
namespace bits {
	using namespace shared;

	class BitWriter {
		public:

		BitWriter(
			std::vector<byte_t>& data
		);

		auto append_bits(
			size_t value,
			size_t width
		) -> void;

		auto flush_bits(
		) -> void;

		protected:

		std::vector<byte_t>& data;
		byte_t current_byte;
		size_t bits_in_byte;
	};
}
}
