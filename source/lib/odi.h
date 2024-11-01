#pragma once

#include <string>
#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace odi {
	using namespace shared;

	const auto MAJOR_VERSION = size_t(1);
	const auto MINOR_VERSION = size_t(0);

	namespace SectorDataCompressionMethod {
		using type = ui08_t;

		const auto NONE = type(0x00);
		const auto GENERIC_LOSSLESS = type(0x01);
		const auto LOSSLESS_STEREO_AUDIO = type(0x80);

		auto name(
			type value
		) -> const std::string&;
	}

	namespace SubchannelsDataCompressionMethod {
		using type = ui08_t;

		const auto NONE = type(0x00);
		const auto GENERIC_LOSSLESS = type(0x01);

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
		ui08_t : 8;
		ui08_t : 8;
		ui32_t point_table_header_absolute_offset;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(FileHeader) == 32);

	struct SectorDataCompressionHeader {
		ui16_t compressed_byte_count;
		SectorDataCompressionMethod::type compression_method;
	};

	static_assert(sizeof(SectorDataCompressionHeader) == 3);

	struct SubchannelsDataCompressionHeader {
		ui16_t compressed_byte_count;
		SubchannelsDataCompressionMethod::type compression_method;
	};

	static_assert(sizeof(SubchannelsDataCompressionHeader) == 3);

	struct SectorTableEntry {
		ui32_t compressed_data_absolute_offset;
		ui08_t : 8;
		ui08_t : 8;
		Readability::type readability;
		SectorDataCompressionHeader sector_data;
		SubchannelsDataCompressionHeader subchannels_data;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(SectorTableEntry) == 16);

	struct SectorTableHeader {
		ui32_t entry_count;
		ui16_t entry_length = sizeof(SectorTableEntry);
		ui16_t header_length = sizeof(SectorTableHeader);
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(SectorTableHeader) == 16);

	struct PointTableEntry {
		byte_t entry[11];
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(PointTableEntry) == 16);

	struct PointTableHeader {
		ui32_t entry_count;
		ui16_t entry_length = sizeof(PointTableEntry);
		ui16_t header_length = sizeof(PointTableHeader);
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(PointTableHeader) == 16);

	struct LosslessStereoAudioHeader {
		ui16_t header_length = sizeof(LosslessStereoAudioHeader);
		ui08_t k;
		ui08_t r_predictor_index;
		ui08_t l_predictor_index;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
	};

	static_assert(sizeof(LosslessStereoAudioHeader) == 8);

	struct GenericLosslessHeader {
		ui16_t header_length = sizeof(GenericLosslessHeader);
		ui08_t k;
		ui08_t : 8;
	};

	static_assert(sizeof(GenericLosslessHeader) == 4);

	#pragma pack(pop)

	auto compress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		SectorDataCompressionMethod::type compression_method
	) -> size_t;

	auto decompress_sector_data(
		array<cd::SECTOR_LENGTH, byte_t>& sector_data,
		size_t compressed_byte_count,
		SectorDataCompressionMethod::type compression_method
	) -> void;

	auto compress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		SubchannelsDataCompressionMethod::type compression_method
	) -> size_t;

	auto decompress_subchannels_data(
		array<cd::SUBCHANNELS_LENGTH, byte_t>& subchannels_data,
		size_t compressed_byte_count,
		SubchannelsDataCompressionMethod::type compression_method
	) -> void;
}
}
