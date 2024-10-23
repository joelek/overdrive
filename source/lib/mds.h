#pragma once

#include "cdb.h"
#include "disc.h"
#include "shared.h"

namespace overdrive {
namespace mds {
	using namespace shared;

	namespace TrackMode {
		using type = ui08_t;

		const auto NONE = type(0x0);
		const auto UNKNOWN_1 = type(0x1);
		const auto DVD = type(0x2);
		const auto UNKNOWN_3 = type(0x3);
		const auto UNKNOWN_4 = type(0x4);
		const auto UNKNOWN_5 = type(0x5);
		const auto UNKNOWN_6 = type(0x6);
		const auto UNKNOWN_7 = type(0x7);
		const auto UNKNOWN_8 = type(0x8);
		const auto AUDIO = type(0x9);
		const auto MODE1 = type(0xA);
		const auto MODE2 = type(0xB);
		const auto MODE2_FORM1 = type(0xC);
		const auto MODE2_FORM2 = type(0xD);
		const auto UNKNOWN_E = type(0xE);
		const auto UNKNOWN_F = type(0xF);
	}

	namespace TrackModeFlags {
		using type = ui08_t;

		const auto UNKNOWN_0 = type(0x0);
		const auto UNKNOWN_1 = type(0x1);
		const auto UNKNOWN_2 = type(0x2);
		const auto UNKNOWN_3 = type(0x3);
		const auto UNKNOWN_4 = type(0x4);
		const auto UNKNOWN_5 = type(0x5);
		const auto UNKNOWN_6 = type(0x6);
		const auto UNKNOWN_7 = type(0x7);
		const auto UNKNOWN_8 = type(0x8);
		const auto UNKNOWN_9 = type(0x9);
		const auto UNKNOWN_A = type(0xA);
		const auto UNKNOWN_B = type(0xB);
		const auto UNKNOWN_C = type(0xC);
		const auto UNKNOWN_D = type(0xD);
		const auto UNKNOWN_E = type(0xE);
		const auto UNKNOWN_F = type(0xF);
	}

	namespace SubchannelMode {
		using type = ui08_t;

		const auto NONE = type(0x00);
		const auto INTERLEAVED_96 = type(0x08);
	}

	#pragma pack(push, 1)

	struct FileHeader {
		ch08_t identifier[16] = { 'M','E','D','I','A',' ','D','E','S','C','R','I','P','T','O','R' };
		ui08_t major_version = 1;
		ui08_t minor_version = 3;
		ui16_t medium_type;
		ui16_t session_count;
		ui16_t unknown_a;
		ui08_t unknown_b[56];
		ui32_t absolute_offset_to_session_headers;
		ui32_t absolute_offset_to_footer;
	};

	static_assert(sizeof(FileHeader) == 88);

	struct SessionTableHeader {
		si32_t pregap_correction;
		ui32_t sectors_on_disc;
		ui16_t session_number;
		ui08_t point_count;
		ui08_t non_track_point_count;
		ui16_t first_track;
		ui16_t last_track;
		ui08_t unknown_a[4];
		ui32_t absolute_offset_to_entry_table;
	};

	static_assert(sizeof(SessionTableHeader) == 24);

	struct SessionTableEntry {
		TrackMode::type track_mode : 4;
		TrackModeFlags::type track_mode_flags: 4;
		union {
			cdb::ReadTOCResponseFullTOCEntry entry;
			SubchannelMode::type subchannel_mode;
		};
		ui32_t absolute_offset_to_track_table_entry;
		ui16_t sector_length;
		ui16_t unknown_a;
		ui08_t unknown_b[16];
		ui32_t first_sector_on_disc;
		ui32_t mdf_byte_offset;
		ui08_t unknown_c[4];
		ui32_t unknown_d;
		ui32_t absolute_offset_to_file_table_header;
		ui08_t unknown_e[24];
	};

	static_assert(sizeof(SessionTableEntry) == 80);

	struct TrackTableHeader {
		ui08_t unknown_a[24];
	};

	static_assert(sizeof(TrackTableHeader) == 24);

	struct TrackTableEntry {
		ui32_t pregap_sectors;
		ui32_t length_sectors;
	};

	static_assert(sizeof(TrackTableEntry) == 8);

	struct FileTableHeader {
		ui32_t absolute_offset_to_file_table_entry;
		ui08_t unknown_a[12];
	};

	static_assert(sizeof(FileTableHeader) == 16);

	struct FileTableEntry {
		ch08_t identifier[6] = "*.mdf";
	};

	static_assert(sizeof(FileTableEntry) == 6);

	struct FileFooter {
		ui32_t unknown_a;
		ui32_t absolute_offset_to_bad_sectors_table_header;
	};

	static_assert(sizeof(FileFooter) == 8);

	struct BadSectorTableHeader {
		ui32_t unknown_a;
		ui32_t unknown_b;
		ui32_t unknown_c;
		ui32_t bad_sector_count;
	};

	static_assert(sizeof(BadSectorTableHeader) == 16);

	struct BadSectorTableEntry {
		ui32_t bad_sector_index;
	};

	static_assert(sizeof(BadSectorTableEntry) == 4);

	#pragma pack(pop)

	auto get_track_mode(
		disc::TrackType::type track_type
	) -> TrackMode::type;

	auto get_track_mode_flags(
		disc::TrackType::type track_type
	) -> TrackModeFlags::type;
}
}
