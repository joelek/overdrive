#include "cd.h"

#include "exceptions.h"

namespace overdrive {
namespace cd {
	auto get_absolute_sector_index(
		si_t relative_sector_index
	) -> ui_t {
		auto absolute_sector_index = ui_t(relative_sector_index) + 150;
		return absolute_sector_index;
	}

	auto get_relative_sector_index(
		ui_t absolute_sector_index
	) -> si_t {
		auto relative_sector_index = si_t(absolute_sector_index) - 150;
		return relative_sector_index;
	}

	auto get_sector_from_address(
		const SectorAddress& address
	) -> ui_t {
		auto sector = (address.m * MINUTES_PER_SECOND + address.s) * SECTORS_PER_SECOND + address.f;
		if (sector > MAX_SECTOR) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("sector", sector, 0, MAX_SECTOR));
		}
		return sector;
	}

	auto get_address_from_sector(
		ui_t sector
	) -> SectorAddress {
		if (sector > MAX_SECTOR) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("sector", sector, 0, MAX_SECTOR));
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
		auto subchannels = Subchannels();
		for (auto subchannel_index = 7; subchannel_index >= 0; subchannel_index -= 1) {
			auto shift = 7 - subchannel_index;
			auto offset = 0;
			for (auto byte_index = 0; byte_index < si_t(SUBCHANNEL_LENGTH); byte_index += 1) {
				auto byte = 0;
				for (auto bit_index = 0; bit_index < 8; bit_index += 1) {
					auto subchannel_byte = data[offset];
					auto subchannel_bit = (subchannel_byte >> shift) & 1;
					byte <<= 1;
					byte |= subchannel_bit;
					offset += 1;
				}
				subchannels.channels[subchannel_index][byte_index] = byte;
			}
		}
		return subchannels;
	}

	auto get_track_category(
		ui_t control
	) -> TrackCategory {
		auto category = (control >> 2) & 0b11;
		if (category == 0b00) {
			return TrackCategory::AUDIO_2_CHANNELS;
		}
		if (category == 0b01) {
			return TrackCategory::DATA;
		}
		if (category == 0b10) {
			return TrackCategory::AUDIO_4_CHANNELS;
		}
		if (category == 0b11) {
			return TrackCategory::RESERVED;
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto is_audio_category(
		TrackCategory category
	) -> bool_t {
		if (category == TrackCategory::AUDIO_2_CHANNELS) {
			return true;
		}
		if (category == TrackCategory::DATA) {
			return false;
		}
		if (category == TrackCategory::AUDIO_4_CHANNELS) {
			return true;
		}
		if (category == TrackCategory::RESERVED) {
			return false;
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
