#pragma once

#include "type.h"

namespace overdrive {
namespace wav {
	using namespace type;

	#pragma pack(push, 1)

	struct Header {
		ch08_t riff_identifier[4] = { 'R', 'I', 'F', 'F' };
		ui32_t riff_length = 44 - 8 + 0;
		ch08_t wave_identifier[4] = { 'W', 'A', 'V', 'E' };
		ch08_t format_identifier[4] = { 'f', 'm', 't', ' ' };
		ui32_t format_data_length = 16;
		ui16_t format = 1;
		ui16_t number_of_channels = 2;
		ui32_t sample_rate_hz = 44100;
		ui32_t data_rate_bytes_per_second = (16 * 2 * 44100) >> 3;
		ui16_t bytes_per_sample = (16 * 2) >> 3;
		ui16_t bits_per_sample = 16;
		ch08_t data_identifier[4] = { 'd', 'a', 't', 'a' };
		ui32_t data_length = 0;
	};

	static_assert(sizeof(Header) == 44);

	#pragma pack(pop)
}
}
