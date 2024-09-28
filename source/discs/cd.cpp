#include "cd.h"

#include "../lib/exceptions.h"

namespace discs {
namespace cd {
	auto get_sector_from_address(
		const SectorAddress& address
	) -> ui_t {
		auto sector = (address.m * MINUTES_PER_SECOND + address.s) * SECTORS_PER_SECOND + address.f;
		if (sector > MAX_SECTOR) {
			throw overdrive::exceptions::InvalidValueException(sector, 0, MAX_SECTOR);
		}
		return sector;
	}

	auto get_address_from_sector(
		ui_t sector
	) -> SectorAddress {
		if (sector > MAX_SECTOR) {
			throw overdrive::exceptions::InvalidValueException(sector, 0, MAX_SECTOR);
		}
		auto f = sector;
		auto s = sector / SECTORS_PER_SECOND;
		f -= s * SECTORS_PER_SECOND;
		auto m = s / MINUTES_PER_SECOND;
		s -= m * MINUTES_PER_SECOND;
		auto address = SectorAddress();
		address.m = m;
		address.s = s;
		address.f = f;
		return address;
	}

	auto deinterleave_subchannel_data(
		reference<array<SUBCHANNELS_LENGTH, byte_t>> data
	) -> Subchannels {
		auto subchannel_data = Subchannels();
		for (auto subchannel_index = 7; subchannel_index >= 0; subchannel_index -= 1) {
			auto shift = 7 - subchannel_index;
			auto offset = 0;
			for (auto byte_index = 0; byte_index < SUBCHANNEL_LENGTH; byte_index += 1) {
				auto byte = 0;
				for (auto bit_index = 0; bit_index < 8; bit_index += 1) {
					auto subchannel_byte = data[offset];
					auto subchannel_bit = (subchannel_byte >> shift) & 1;
					byte <<= 1;
					byte |= subchannel_bit;
					offset += 1;
				}
				subchannel_data.channels[subchannel_index][byte_index] = byte;
			}
		}
		return subchannel_data;
	}
}
}
