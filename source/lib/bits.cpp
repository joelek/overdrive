#include "bits.h"

#include <bit>
#include "exceptions.h"

namespace overdrive {
namespace bits {
	namespace internal {
	namespace {
		auto encode_value_using_exponential_golomb_coding(
			size_t value,
			size_t k,
			BitWriter& bitwriter
		) -> void {
			auto power = size_t(1) << k;
			auto exponential_value = value + power;
			auto width = sizeof(exponential_value) * 8 - std::countl_zero(exponential_value);
			bitwriter.append_bits(0, width - 1 - k);
			bitwriter.append_bits(exponential_value, width);
		}

		auto decode_value_using_exponential_golomb_coding(
			size_t k,
			BitReader& bitreader
		) -> size_t {
			auto power = size_t(1) << k;
			auto width = size_t(k + 1);
			auto exponential_value = size_t(0);
			while (true) {
				exponential_value = bitreader.decode_bits(1);
				if (exponential_value != 0) {
					break;
				}
				width += 1;
			}
			width -= 1;
			if (width > 0) {
				exponential_value = (exponential_value << width) | bitreader.decode_bits(width);
			}
			auto value = exponential_value - power;
			return value;
		}
	}
	}

	BitReader::BitReader(
		const std::vector<byte_t>& buffer,
		size_t offset
	):
		buffer(buffer),
		offset(offset),
		current_byte(0),
		bits_in_byte(0),
		mask(0b00000000)
	{

	}

	auto BitReader::decode_bits(
		size_t width
	) -> size_t {
		auto value = size_t(0);
		for (auto bit = size_t(0); bit < width; bit += 1) {
			if (this->bits_in_byte == 0) {
				this->current_byte = this->buffer.at(offset);
				this->bits_in_byte = 8;
				this->offset += 1;
				this->mask = 0b10000000;
			}
			value = (value << 1) | ((this->current_byte & this->mask) ? 1 : 0);
			this->bits_in_byte -= 1;
			this->mask >>= 1;
		}
		return value;
	}

	auto BitReader::get_size(
	) const -> size_t {
		return this->buffer.size() * 8;
	}

	BitWriter::BitWriter(
		std::optional<size_t> max_size
	):
		buffer(std::vector<byte_t>()),
		max_size(max_size),
		current_byte(0),
		bits_in_byte(0)
	{

	}

	auto BitWriter::append_bits(
		size_t value,
		size_t width
	) -> void {
		if (width > 0) {
			auto mask = (size_t(1) << (width - 1));
			for (auto bit = size_t(0); bit < width; bit += 1) {
				this->current_byte |= ((value & mask) ? 1 : 0) << (8 - bits_in_byte - 1);
				this->bits_in_byte += 1;
				if (this->bits_in_byte == 8) {
					this->flush_bits();
				}
				mask >>= 1;
			}
		}
	}

	auto BitWriter::append_one(
	) -> void {
		this->current_byte |= 1 << (8 - bits_in_byte - 1);
		this->bits_in_byte += 1;
		if (this->bits_in_byte == 8) {
			this->flush_bits();
		}
	}

	auto BitWriter::append_zero(
	) -> void {
		this->bits_in_byte += 1;
		if (this->bits_in_byte == 8) {
			this->flush_bits();
		}
	}

	auto BitWriter::flush_bits(
	) -> void {
		if (this->bits_in_byte > 0) {
			this->buffer.push_back(this->current_byte);
			this->current_byte = 0;
			this->bits_in_byte = 0;
			if (this->max_size && this->buffer.size() > this->max_size.value()) {
				OVERDRIVE_THROW(exceptions::BitWriterSizeExceededError(this->max_size.value()));
			}
		}
	}

	auto BitWriter::get_buffer(
	) -> std::vector<byte_t>& {
		return this->buffer;
	}

	auto BitWriter::get_size(
	) const -> size_t {
		return this->buffer.size() * 8 + this->bits_in_byte;
	}

	auto compress_data_using_exponential_golomb_coding(
		const ui16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void {
		auto power = size_t(1) << k;
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto unsigned_value = values[value_index];
			auto exponential_value = unsigned_value + power;
			auto width = sizeof(exponential_value) * 8 - std::countl_zero(exponential_value);
			bitwriter.append_bits(0, width - 1 - k);
			bitwriter.append_bits(exponential_value, width);
		}
	}

	auto compress_data_using_exponential_golomb_coding(
		const si16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void {
		auto power = size_t(1) << k;
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto value = values[value_index];
			auto unsigned_value = ui16_t(value < 0 ? 0 - (value << 1) - 1 : value << 1);
			auto exponential_value = unsigned_value + power;
			auto width = sizeof(exponential_value) * 8 - std::countl_zero(exponential_value);
			bitwriter.append_bits(0, width - 1 - k);
			bitwriter.append_bits(exponential_value, width);
		}
	}

	auto compress_data_using_rice_coding(
		const ui16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void {
		auto mask = size_t((1 << k) - 1);
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto unsigned_value = values[value_index];
			auto quotient = size_t(unsigned_value >> k);
			auto remainder = size_t(unsigned_value & mask);
			for (auto bit_index = size_t(0); bit_index < quotient; bit_index += 1) {
				bitwriter.append_zero();
			}
			bitwriter.append_one();
			bitwriter.append_bits(remainder, k);
		}
	}

	auto compress_data_using_rice_coding(
		const si16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void {
		auto mask = size_t((1 << k) - 1);
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto value = values[value_index];
			auto unsigned_value = ui16_t(value < 0 ? 0 - (value << 1) - 1 : value << 1);
			auto quotient = size_t(unsigned_value >> k);
			auto remainder = size_t(unsigned_value & mask);
			for (auto bit_index = size_t(0); bit_index < quotient; bit_index += 1) {
				bitwriter.append_zero();
			}
			bitwriter.append_one();
			bitwriter.append_bits(remainder, k);
		}
	}

	auto compress_data_using_rle_coding(
		const byte_t* bytes,
		size_t size,
		BitWriter& bitwriter
	) -> void {
		auto offset = size_t(0);
		while (offset < size) {
			auto raw_length = size_t(1);
			for (auto byte_index = offset + 1; byte_index < size; byte_index += 1) {
				if (bytes[byte_index] != bytes[byte_index - 1]) {
					raw_length += 1;
				} else {
					raw_length -= 1;
					break;
				}
			}
			if (raw_length > 0) {
				bitwriter.append_zero();
				internal::encode_value_using_exponential_golomb_coding(raw_length - 1, 0, bitwriter);
				for (auto byte_index = offset; byte_index < offset + raw_length; byte_index += 1) {
					bitwriter.append_bits(bytes[byte_index], 8);
				}
				offset += raw_length;
			}
			auto run_length = size_t(1);
			for (auto byte_index = offset + 1; byte_index < size; byte_index += 1) {
				if (bytes[byte_index] == bytes[byte_index - 1]) {
					run_length += 1;
				} else {
					break;
				}
			}
			if (run_length > 1) {
				bitwriter.append_one();
				internal::encode_value_using_exponential_golomb_coding(run_length - 2, 0, bitwriter);
				bitwriter.append_bits(bytes[offset], 8);
				offset += run_length;
			}
		}
	}

	auto decompress_data_using_exponential_golomb_coding(
		ui16_t* values,
		size_t size,
		size_t k,
		BitReader& bitreader
	) -> void {
		auto power = size_t(1) << k;
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto width = size_t(k + 1);
			auto exponential_value = size_t(0);
			while (true) {
				exponential_value = bitreader.decode_bits(1);
				if (exponential_value != 0) {
					break;
				}
				width += 1;
			}
			width -= 1;
			if (width > 0) {
				exponential_value = (exponential_value << width) | bitreader.decode_bits(width);
			}
			auto unsigned_value = exponential_value - power;
			values[value_index] = unsigned_value;
		}
	}

	auto decompress_data_using_exponential_golomb_coding(
		si16_t* values,
		size_t size,
		size_t k,
		BitReader& bitreader
	) -> void {
		auto power = size_t(1) << k;
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto width = size_t(k + 1);
			auto exponential_value = size_t(0);
			while (true) {
				exponential_value = bitreader.decode_bits(1);
				if (exponential_value != 0) {
					break;
				}
				width += 1;
			}
			width -= 1;
			if (width > 0) {
				exponential_value = (exponential_value << width) | bitreader.decode_bits(width);
			}
			auto unsigned_value = exponential_value - power;
			auto value = si16_t((unsigned_value & 1) ? 0 - ((unsigned_value + 1) >> 1) : unsigned_value >> 1);
			values[value_index] = value;
		}
	}

	auto decompress_data_using_rice_coding(
		ui16_t* values,
		size_t size,
		size_t k,
		BitReader& bitreader
	) -> void {
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto quotient = size_t(0);
			while (true) {
				auto bit = bitreader.decode_bits(1);
				if (bit != 0) {
					break;
				}
				quotient += 1;
			}
			auto remainder = bitreader.decode_bits(k);
			auto unsigned_value = (quotient << k) | remainder;
			values[value_index] = unsigned_value;
		}
	}
	auto decompress_data_using_rice_coding(
		si16_t* values,
		size_t size,
		size_t k,
		BitReader& bitreader
	) -> void {
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto quotient = size_t(0);
			while (true) {
				auto bit = bitreader.decode_bits(1);
				if (bit != 0) {
					break;
				}
				quotient += 1;
			}
			auto remainder = bitreader.decode_bits(k);
			auto unsigned_value = (quotient << k) | remainder;
			auto value = si16_t((unsigned_value & 1) ? 0 - ((unsigned_value + 1) >> 1) : unsigned_value >> 1);
			values[value_index] = value;
		}
	}

	auto decompress_data_using_rle_coding(
		byte_t* bytes,
		size_t size,
		BitReader& bitreader
	) -> void {
		auto offset = size_t(0);
		while (offset < size) {
			auto control = bitreader.decode_bits(1);
			if (control) {
				auto run_length = 2 + internal::decode_value_using_exponential_golomb_coding(0, bitreader);
				auto byte = bitreader.decode_bits(8);
				for (auto byte_count = size_t(0); byte_count < run_length; byte_count += 1) {
					bytes[offset] = byte;
					offset += 1;
				}
			} else {
				auto raw_length = 1 + internal::decode_value_using_exponential_golomb_coding(0, bitreader);
				for (auto byte_count = size_t(0); byte_count < raw_length; byte_count += 1) {
					auto byte = bitreader.decode_bits(8);
					bytes[offset] = byte;
					offset += 1;
				}
			}
		}
	}
}
}
