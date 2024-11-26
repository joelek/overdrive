#include "cd.h"

#include "crc.h"
#include "exceptions.h"

namespace overdrive {
namespace cd {
	auto get_absolute_sector_index(
		si_t relative_sector_index
	) -> si_t {
		auto absolute_sector_index = relative_sector_index + si_t(RELATIVE_SECTOR_OFFSET);
		return absolute_sector_index;
	}

	auto get_relative_sector_index(
		si_t absolute_sector_index
	) -> si_t {
		auto relative_sector_index = absolute_sector_index - si_t(RELATIVE_SECTOR_OFFSET);
		return relative_sector_index;
	}

	auto get_sector_from_address(
		const SectorAddress& address
	) -> si_t {
		if (address.m > MINUTES_PER_DISC) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("minutes", address.m, 0, MINUTES_PER_DISC));
		}
		if (address.s > MINUTES_PER_SECOND - 1) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("seconds", address.m, 0, MINUTES_PER_SECOND - 1));
		}
		if (address.f > SECTORS_PER_SECOND - 1) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("frames", address.m, 0, SECTORS_PER_SECOND - 1));
		}
		auto sector = (address.m * MINUTES_PER_SECOND + address.s) * SECTORS_PER_SECOND + address.f;
		if (sector > MAX_SECTOR) {
			sector -= ADDRESSABLE_SECTOR_COUNT;
		}
		return sector;
	}

	auto get_address_from_sector(
		si_t sector
	) -> SectorAddress {
		if (sector < MIN_SECTOR || sector > MAX_SECTOR) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("sector", sector, MIN_SECTOR, MAX_SECTOR));
		}
		if (sector < 0) {
			sector += ADDRESSABLE_SECTOR_COUNT;
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

	auto deinterleave_subchannels(
		const Subchannels& subchannels
	) -> Subchannels {
		auto deinterleaved = Subchannels();
		for (auto subchannel_index = 7; subchannel_index >= 0; subchannel_index -= 1) {
			auto shift = 7 - subchannel_index;
			auto offset = 0;
			for (auto byte_index = 0; byte_index < si_t(SUBCHANNEL_LENGTH); byte_index += 1) {
				auto byte = 0;
				for (auto bit_index = 0; bit_index < 8; bit_index += 1) {
					auto subchannel_byte = subchannels.data[offset];
					auto subchannel_bit = (subchannel_byte >> shift) & 1;
					byte <<= 1;
					byte |= subchannel_bit;
					offset += 1;
				}
				deinterleaved.channels[subchannel_index].data[byte_index] = byte;
			}
		}
		return deinterleaved;
	}

	auto reinterleave_subchannels(
		const Subchannels& subchannels
	) -> Subchannels {
		auto reinterleaved = Subchannels();
		for (auto byte_index = size_t(0); byte_index < SUBCHANNELS_LENGTH; byte_index += 1) {
			auto subchannel_bit_index = 7 - (byte_index & 7);
			auto subchannel_byte_index = byte_index >> 3;
			auto byte = byte_t(0);
			for (auto subchannel_index = size_t(0); subchannel_index < SUBCHANNEL_COUNT; subchannel_index += 1) {
				auto subchannel_byte = subchannels.channels[subchannel_index].data[subchannel_byte_index];
				auto subchannel_bit = (subchannel_byte >> subchannel_bit_index) & 1;
				byte <<= 1;
				byte |= subchannel_bit;
			}
			reinterleaved.data[byte_index] = byte;
		}
		return reinterleaved;
	}

	auto get_track_category(
		ui_t control
	) -> TrackCategory::type {
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
		TrackCategory::type category
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

	auto is_data_category(
		TrackCategory::type category
	) -> bool_t {
		if (category == TrackCategory::AUDIO_2_CHANNELS) {
			return false;
		}
		if (category == TrackCategory::DATA) {
			return true;
		}
		if (category == TrackCategory::AUDIO_4_CHANNELS) {
			return false;
		}
		if (category == TrackCategory::RESERVED) {
			return false;
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto compute_subchannel_q_crc(
		const SubchannelQ& q
	) -> ui16_t {
		auto crc = crc::compute_crc16(reinterpret_cast<const byte_t*>(&q), offsetof(SubchannelQ, crc_be));
		return crc;
	}
}
}
