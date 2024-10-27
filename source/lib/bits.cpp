#include "bits.h"

namespace overdrive {
namespace bits {
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
		for (auto bit = size_t(0); bit < width; bit += 1) {
			this->current_byte = (this->current_byte << 1) | (value & 1);
			this->bits_in_byte += 1;
			if (this->bits_in_byte == 8) {
				this->flush_bits();
			}
			value >>= 1;
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
