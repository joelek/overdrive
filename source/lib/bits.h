#pragma once

#include <optional>
#include <vector>
#include "shared.h"

namespace overdrive {
namespace bits {
	using namespace shared;

	class BitReader {
		public:

		BitReader(
		) = default;

		BitReader(
			const std::vector<byte_t>& buffer,
			size_t offset
		);

		auto decode_bits(
			size_t width
		) -> size_t;

		protected:

		const std::vector<byte_t>& buffer;
		size_t offset;
		byte_t current_byte;
		size_t bits_in_byte;
		size_t mask;
	};

	class BitWriter {
		public:

		BitWriter(
		) = default;

		BitWriter(
			std::optional<size_t> max_size
		);

		auto append_bits(
			size_t value,
			size_t width
		) -> void;

		auto append_one(
		) -> void;

		auto append_zero(
		) -> void;

		auto flush_bits(
		) -> void;

		auto get_buffer(
		) -> std::vector<byte_t>&;

		auto get_size(
		) const -> size_t;

		protected:

		std::vector<byte_t> buffer;
		std::optional<size_t> max_size;
		byte_t current_byte;
		size_t bits_in_byte;
	};

	auto compress_data_using_exponential_golomb_coding(
		const si16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void;

	auto compress_data_using_rice_coding(
		const si16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void;

	auto decompress_data_using_exponential_golomb_coding(
		si16_t* values,
		size_t size,
		size_t k,
		BitReader& bitreader
	) -> void;

	auto decompress_data_using_rice_coding(
		si16_t* values,
		size_t size,
		size_t k,
		BitReader& bitreader
	) -> void;
}
}
