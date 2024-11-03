#include "bits.h"

#include <bit>
#include <cstring>

namespace overdrive {
namespace bits {
	BitReader::BitReader(
		const std::vector<byte_t>& data,
		size_t offset
	):
		data(data),
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
				this->current_byte = this->data.at(offset);
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

	BitWriter::BitWriter(
		std::vector<byte_t>& data
	):
		data(data),
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
			this->data.push_back(this->current_byte);
			this->current_byte = 0;
			this->bits_in_byte = 0;
		}
	}

	auto compress_data_using_exponential_golomb_coding(
		const ui08_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void {
		auto power = size_t(1) << k;
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto value = values[value_index];
			auto exponential_value = value + power;
			auto width = sizeof(exponential_value) * 8 - std::countl_zero(exponential_value);
			bitwriter.append_bits(0, width - 1 - k);
			bitwriter.append_bits(exponential_value , width);
		}
		bitwriter.flush_bits();
	}

	auto compress_data_using_exponential_golomb_coding(
		const ui16_t* values,
		size_t size,
		size_t k,
		BitWriter& bitwriter
	) -> void {
		auto power = size_t(1) << k;
		for (auto value_index = size_t(0); value_index < size; value_index += 1) {
			auto value = values[value_index];
			auto exponential_value = value + power;
			auto width = sizeof(exponential_value) * 8 - std::countl_zero(exponential_value);
			bitwriter.append_bits(0, width - 1 - k);
			bitwriter.append_bits(exponential_value, width);
		}
		bitwriter.flush_bits();
	}

	auto decompress_data_using_exponential_golomb_coding(
		ui08_t* values,
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
			values[value_index] = exponential_value - power;
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
			values[value_index] = exponential_value - power;
		}
	}
}
}
