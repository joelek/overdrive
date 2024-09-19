#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "type.h"

namespace iso9660 {
	using namespace type;

	const auto PRIMARY_VOLUME_DESCRIPTOR_SECTOR = size_t(16);
	const auto CURRENT_DIRECTORY_IDENTIFIER = std::string("\0", 1);
	const auto PARENT_DIRECTORY_IDENTIFIER = std::string("\1", 1);

	#pragma pack(push, 1)

	struct DirectoryEntryTimestamp {
		ui08_t years_since_1900;
		ui08_t month_number;
		ui08_t day_number;
		ui08_t hours;
		ui08_t minutes;
		ui08_t seconds;
		si08_t time_zone_offset_quarter_hours;
	};

	static_assert(sizeof(DirectoryEntryTimestamp) == 7);

	struct DirectoryEntryFlags {
		ui08_t hidden: 1;
		ui08_t directory: 1;
		ui08_t associated: 1;
		ui08_t extended_record_contains_format: 1;
		ui08_t extended_record_contains_permissions: 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t final: 1;
	};

	static_assert(sizeof(DirectoryEntryFlags) == 1);

	struct DirectoryEntryHeader {
		ui08_t length;
		ui08_t extended_record_length;
		ui32_t extent_sector_le;
		ui32_t extent_sector_be;
		ui32_t data_length_le;
		ui32_t data_length_be;
		DirectoryEntryTimestamp timestamp;
		DirectoryEntryFlags flags;
		ui08_t interleaved_file_unit_size;
		ui08_t interleaved_gap_size;
		ui16_t volume_sequence_number_le;
		ui16_t volume_sequence_number_be;
		ui08_t identifier_length;
	};

	static_assert(sizeof(DirectoryEntryHeader) == 33);

	struct PrimaryVolumeDescriptorTimestamp {
		ch08_t year[4];
		ch08_t month[2];
		ch08_t day[2];
		ch08_t hour[2];
		ch08_t minute[2];
		ch08_t second[2];
		ch08_t hundredths[2];
		si08_t time_zone_offset_quarter_hours;
	};

	static_assert(sizeof(PrimaryVolumeDescriptorTimestamp) == 17);

	enum class VolumeDescriptorType: ui08_t {
		BOOT_RECORD = 0,
		PRIMARY_VOLUME_DESCRIPTOR = 1,
		SUPPLEMENTARY_VOLUME_DESCRIPTOR = 2,
		VOLUME_PARTITION_DESCRIPTOR = 3,
		VOLUME_DESCRIPTOR_SET_TERMINATOR = 255
	};

	static_assert(sizeof(VolumeDescriptorType) == 1);

	struct VolumeDescriptorHeader {
		VolumeDescriptorType type;
		ch08_t identifier[5] = { 'C', 'D', '0', '0', '1' };
		ui08_t version = 1;
	};

	static_assert(sizeof(VolumeDescriptorHeader) == 7);

	struct PrimaryVolumeDescriptor {
		VolumeDescriptorHeader header;
		ui08_t : 8;
		ch08_t system_identifier[32];
		ch08_t volume_identifier[32];
		ui32_t : 32;
		ui32_t : 32;
		ui32_t volume_space_size_le;
		ui32_t volume_space_size_be;
		ui32_t : 32;
		ui32_t : 32;
		ui32_t : 32;
		ui32_t : 32;
		ui32_t : 32;
		ui32_t : 32;
		ui32_t : 32;
		ui32_t : 32;
		ui16_t volume_set_size_le;
		ui16_t volume_set_size_be;
		ui16_t volume_sequence_number_le;
		ui16_t volume_sequence_number_be;
		ui16_t logical_block_size_le;
		ui16_t logical_block_size_be;
		ui32_t path_table_size_le;
		ui32_t path_table_size_be;
		ui32_t location_of_type_l_path_table_le;
		ui32_t location_of_optional_type_l_path_table_le;
		ui32_t location_of_type_m_path_table_be;
		ui32_t location_of_optional_type_m_path_table_be;
		struct {
			DirectoryEntryHeader header;
			ch08_t identifier[1] = { '\0' };
		} root_directory_entry;
		ch08_t volume_set_identifier[128];
		ch08_t publisher_identifier[128];
		ch08_t data_preparer_identifier[128];
		ch08_t application_identifier[128];
		ch08_t copyright_file_identifier[37];
		ch08_t abstract_file_identifier[37];
		ch08_t bibliographic_file_identifier[37];
		PrimaryVolumeDescriptorTimestamp volume_creation_timestamp;
		PrimaryVolumeDescriptorTimestamp volume_modification_timestamp;
		PrimaryVolumeDescriptorTimestamp volume_expiration_timestamp;
		PrimaryVolumeDescriptorTimestamp volume_effective_timestamp;
		ui08_t file_structure_version = 1;
		ui08_t : 8;
		ui08_t application_data[512];
		ui08_t reserved_by_iso[653];
	};

	static_assert(sizeof(PrimaryVolumeDescriptor) == 2048);

	#pragma pack(pop)

	class FileSystemEntry {
		public:

		std::string identifier;
		bool_t is_directory;
		size_t first_sector;
		size_t length_bytes;
		std::optional<size_t> parent_first_sector;
	};

	class FileSystem {
		public:

		FileSystem(
			const std::function<void(size_t sector, void* user_data)>& read_user_data
		);

		auto get_children(
			const FileSystemEntry& entry
		) -> const std::vector<FileSystemEntry>&;

		auto get_entry(
			size_t sector
		) -> std::optional<FileSystemEntry>;

		auto get_hierarchy(
			const FileSystemEntry& entry
		) -> const std::vector<FileSystemEntry>&;

		auto get_root(
		) -> const FileSystemEntry&;

		protected:

		FileSystemEntry root;
		std::vector<FileSystemEntry> entries;
		std::map<size_t, std::vector<FileSystemEntry>> children;
		std::map<size_t, std::vector<FileSystemEntry>> hierarchies;
	};
}
