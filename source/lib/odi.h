#pragma once

#include <string>
#include "shared.h"

namespace overdrive {
namespace odi {
	using namespace shared;

	namespace CompressionMethod {
		using type = ui08_t;

		const auto NONE = type(0x00);

		auto name(
			type value
		) -> const std::string&;
	}

	namespace Readability {
		using type = ui08_t;

		const auto UNREADABLE = type(0x00);
		const auto READABLE = type(0x01);

		auto name(
			type value
		) -> const std::string&;
	}

	#pragma pack(push, 1)

	struct FileHeader {
		ch08_t identifier[16] = "OVERDRIVE IMAGE";
		ui08_t major_version;
		ui08_t minor_version;
		ui08_t patch_version;
		ui08_t : 8;
		ui32_t sector_table_header_absolute_offset;
	};

	static_assert(sizeof(FileHeader) == 24);

	struct SectorTableHeader {
		ui32_t entry_count;
		ui08_t entry_length;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(SectorTableHeader) == 8);

	struct SectorTableEntry {
		ui32_t compressed_data_absolute_offset;
		ui16_t compressed_byte_count;
		CompressionMethod::type compression_method;
		Readability::type readability;
	};

	static_assert(sizeof(SectorTableEntry) == 8);

	struct UncompressedSector {
		byte_t sector_data[2352];
		byte_t subchannels_data[96];
	};

	static_assert(sizeof(UncompressedSector) == 2448);

	#pragma pack(pop)
}
}
