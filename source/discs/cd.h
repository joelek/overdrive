#pragma once

#include "../type.h"

namespace cd {
	using namespace type;

	const auto SECTOR_LENGTH = size_t(2352);
	const auto SECTORS_PER_SECOND = size_t(75);
	const auto SUBCHANNEL_COUNT = size_t(8);
	const auto SUBCHANNEL_LENGTH = size_t(12);
	const auto SUBCHANNEL_P_INDEX = size_t(0);
	const auto SUBCHANNEL_Q_INDEX = size_t(1);
	const auto SUBCHANNEL_R_INDEX = size_t(2);
	const auto SUBCHANNEL_S_INDEX = size_t(3);
	const auto SUBCHANNEL_T_INDEX = size_t(4);
	const auto SUBCHANNEL_U_INDEX = size_t(5);
	const auto SUBCHANNEL_V_INDEX = size_t(6);
	const auto SUBCHANNEL_W_INDEX = size_t(7);
	const auto SUBCHANNELS_LENGTH = size_t(SUBCHANNEL_COUNT * SUBCHANNEL_LENGTH);
	const auto C2_ERROR_POINTERS_LENGTH = size_t(SECTOR_LENGTH / 8);

	#pragma pack(push, 1)

	enum class SessionType: ui08_t {
		CDDA_OR_CDROM = 0x00,
		CDI = 0x10,
		CDXA_OR_DDCD = 0x20
	};

	static_assert(sizeof(SessionType) == 1);

	struct SectorAddress {
		ui08_t m;
		ui08_t s;
		ui08_t f;
	};

	static_assert(sizeof(SectorAddress) == 3);

	struct SubchannelData {
		byte_t channels[SUBCHANNEL_COUNT][SUBCHANNEL_LENGTH];
	};

	static_assert(sizeof(SubchannelData) == SUBCHANNELS_LENGTH);

	enum class Control: ui08_t {
		AUDIO_2_CHANNELS_COPY_PROTECTED = 0b0000,
		AUDIO_2_CHANNELS_COPY_PROTECTED_WITH_PRE_EMPHASIS = 0b0001,
		AUDIO_2_CHANNELS_COPY_PERMITTED = 0b0010,
		AUDIO_2_CHANNELS_COPY_PERMITTED_WITH_PRE_EMPHASIS = 0b0011,
		DATA_COPY_PROTECTED_RECORDED_UINTERRUPTED = 0b0100,
		DATA_COPY_PROTECTED_RECORDED_INCREMENTALLY = 0b0101,
		DATA_COPY_PERMITTED_RECORDED_UINTERRUPTED = 0b0110,
		DATA_COPY_PERMITTED_RECORDED_INCREMENTALLY = 0b0111,
		AUDIO_4_CHANNELS_COPY_PROTECTED = 0b1000,
		AUDIO_4_CHANNELS_COPY_PROTECTED_WITH_PRE_EMPHASIS = 0b1001,
		AUDIO_4_CHANNELS_COPY_PERMITTED = 0b1010,
		AUDIO_4_CHANNELS_COPY_PERMITTED_WITH_PRE_EMPHASIS = 0b1011,
		RESERVED_1100 = 0b1100,
		RESERVED_1101 = 0b1101,
		RESERVED_1110 = 0b1110,
		RESERVED_1111 = 0b1111
	};

	static_assert(sizeof(Control) == 1);

	struct SubchannelQMode1Data {
		ui08_t track_number;
		ui08_t track_index;
		cd::SectorAddress relative_address_bcd;
		ui08_t zero;
		cd::SectorAddress absolute_address_bcd;
	};

	static_assert(sizeof(SubchannelQMode1Data) == 9);

	struct SubchannelQMode2Data {
		ui08_t mcnr02: 4;
		ui08_t mcnr01: 4;
		ui08_t mcnr04: 4;
		ui08_t mcnr03: 4;
		ui08_t mcnr06: 4;
		ui08_t mcnr05: 4;
		ui08_t mcnr08: 4;
		ui08_t mcnr07: 4;
		ui08_t mcnr10: 4;
		ui08_t mcnr09: 4;
		ui08_t mcnr12: 4;
		ui08_t mcnr11: 4;
		ui08_t zero01: 4;
		ui08_t mcnr13: 4;
		ui08_t zero02: 8;
		ui08_t absolute_frame: 8;
	};

	static_assert(sizeof(SubchannelQMode2Data) == 9);

	struct SubchannelQMode3Data {
		ui32_t zero01: 2;
		ui32_t isrc05: 6;
		ui32_t isrc04: 6;
		ui32_t isrc03: 6;
		ui32_t isrc02: 6;
		ui32_t isrc01: 6;
		ui08_t isrc07: 4;
		ui08_t isrc06: 4;
		ui08_t isrc09: 4;
		ui08_t isrc08: 4;
		ui08_t isrc11: 4;
		ui08_t isrc10: 4;
		ui08_t zero02: 4;
		ui08_t isrc12: 4;
		ui08_t absolute_frame: 8;
	};

	static_assert(sizeof(SubchannelQMode3Data) == 9);

	struct SubchannelQ {
		ui08_t adr: 4;
		Control control: 4;
		union {
			SubchannelQMode1Data mode1;
			SubchannelQMode2Data mode2;
			SubchannelQMode3Data mode3;
		};
		ui16_t crc_be;
	};

	static_assert(sizeof(SubchannelQ) == cd::SUBCHANNEL_LENGTH);

	#pragma pack(pop)
}
