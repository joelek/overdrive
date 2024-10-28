#include "bits.h"

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

	auto BitWriter::flush_bits(
	) -> void {
		if (this->bits_in_byte > 0) {
			this->data.push_back(this->current_byte);
			this->current_byte = 0;
			this->bits_in_byte = 0;
		}
	}
}
}
