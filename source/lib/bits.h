#pragma once

#include <vector>
#include "shared.h"

namespace overdrive {
namespace bits {
	using namespace shared;

	class BitReader {
		public:

		BitReader(
			const std::vector<byte_t>& data,
			size_t offset
		);

		auto decode_bits(
			size_t width
		) -> size_t;

		protected:

		const std::vector<byte_t>& data;
		size_t offset;
		byte_t current_byte;
		size_t bits_in_byte;
		size_t mask;
	};

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
