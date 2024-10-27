#pragma once

#include <string>
#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace odi {
	using namespace shared;

	const auto MAJOR_VERSION = size_t(1);
	const auto MINOR_VERSION = size_t(0);

	namespace CompressionMethod {
		using type = ui08_t;

		const auto NONE = type(0x00);
		const auto LOSSLESS_STEREO_AUDIO = type(0x01);

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
		ui08_t major_version = MAJOR_VERSION;
		ui08_t minor_version = MINOR_VERSION;
		ui16_t header_length = sizeof(FileHeader);
		ui32_t sector_table_header_absolute_offset;
		ui32_t point_table_header_absolute_offset;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(FileHeader) == 32);

	struct SectorTableHeader {
		ui32_t entry_count;
		ui16_t entry_length;
		ui16_t header_length = sizeof(SectorTableHeader);
	};

	static_assert(sizeof(SectorTableHeader) == 8);

	struct SectorTableEntry {
		ui32_t compressed_data_absolute_offset;
		ui16_t compressed_byte_count;
		CompressionMethod::type compression_method;
		Readability::type readability;
	};

	static_assert(sizeof(SectorTableEntry) == 8);

	struct PointTableHeader {
		ui32_t entry_count;
		ui16_t entry_length;
		ui16_t header_length = sizeof(PointTableHeader);
	};

	static_assert(sizeof(PointTableHeader) == 8);

	struct PointTableEntry {
		byte_t entry[11];
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(PointTableEntry) == 16);

	struct UncompressedSector {
		byte_t sector_data[cd::SECTOR_LENGTH];
		byte_t subchannels_data[cd::SUBCHANNELS_LENGTH];
	};

	static_assert(sizeof(UncompressedSector) == 2448);

	#pragma pack(pop)

	auto compress_sector(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		CompressionMethod::type compression_method
	) -> size_t;

	auto decompress_sector(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		size_t compressed_byte_count,
		CompressionMethod::type compression_method
	) -> void;
}
}
