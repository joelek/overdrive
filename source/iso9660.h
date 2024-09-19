#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace iso9660 {
	const auto PRIMARY_VOLUME_DESCRIPTOR_SECTOR = size_t(16);
	const auto CURRENT_DIRECTORY_IDENTIFIER = std::string("\0", 1);
	const auto PARENT_DIRECTORY_IDENTIFIER = std::string("\1", 1);

	#pragma pack(push, 1)

	typedef struct {
		uint8_t years_since_1900;
		uint8_t month_number;
		uint8_t day_number;
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
		int8_t time_zone_offset_quarter_hours;
	} DirectoryEntryTimestamp;

	typedef struct {
		uint8_t hidden: 1;
		uint8_t directory: 1;
		uint8_t associated: 1;
		uint8_t extended_record_contains_format: 1;
		uint8_t extended_record_contains_permissions: 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t final: 1;
	} DirectoryEntryFlags;

	typedef struct {
		uint8_t length;
		uint8_t extended_record_length;
		uint32_t extent_sector_le;
		uint32_t extent_sector_be;
		uint32_t data_length_le;
		uint32_t data_length_be;
		DirectoryEntryTimestamp timestamp;
		DirectoryEntryFlags flags;
		uint8_t interleaved_file_unit_size;
		uint8_t interleaved_gap_size;
		uint16_t volume_sequence_number_le;
		uint16_t volume_sequence_number_be;
		uint8_t identifier_length;
	} DirectoryEntryHeader;

	typedef struct {
		char year[4];
		char month[2];
		char day[2];
		char hour[2];
		char minute[2];
		char second[2];
		char hundredths[2];
		int8_t time_zone_offset_quarter_hours;
	} PrimaryVolumeDescriptorTimestamp;

	enum class VolumeDescriptorType: uint8_t {
		BOOT_RECORD = 0,
		PRIMARY_VOLUME_DESCRIPTOR = 1,
		SUPPLEMENTARY_VOLUME_DESCRIPTOR = 2,
		VOLUME_PARTITION_DESCRIPTOR = 3,
		VOLUME_DESCRIPTOR_SET_TERMINATOR = 255
	};

	typedef struct {
		VolumeDescriptorType type;
		char identifier[5] = { 'C', 'D', '0', '0', '1' };
		uint8_t version = 1;
	} VolumeDescriptorHeader;

	typedef struct {
		VolumeDescriptorHeader header;
		uint8_t : 8;
		char system_identifier[32];
		char volume_identifier[32];
		uint32_t : 32;
		uint32_t : 32;
		uint32_t volume_space_size_le;
		uint32_t volume_space_size_be;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint32_t : 32;
		uint16_t volume_set_size_le;
		uint16_t volume_set_size_be;
		uint16_t volume_sequence_number_le;
		uint16_t volume_sequence_number_be;
		uint16_t logical_block_size_le;
		uint16_t logical_block_size_be;
		uint32_t path_table_size_le;
		uint32_t path_table_size_be;
		uint32_t location_of_type_l_path_table_le;
		uint32_t location_of_optional_type_l_path_table_le;
		uint32_t location_of_type_m_path_table_be;
		uint32_t location_of_optional_type_m_path_table_be;
		struct {
			DirectoryEntryHeader header;
			char identifier[1];
		} root_directory_entry;
		char volume_set_identifier[128];
		char publisher_identifier[128];
		char data_preparer_identifier[128];
		char application_identifier[128];
		char copyright_file_identifier[37];
		char abstract_file_identifier[37];
		char bibliographic_file_identifier[37];
		PrimaryVolumeDescriptorTimestamp volume_creation_timestamp;
		PrimaryVolumeDescriptorTimestamp volume_modification_timestamp;
		PrimaryVolumeDescriptorTimestamp volume_expiration_timestamp;
		PrimaryVolumeDescriptorTimestamp volume_effective_timestamp;
		uint8_t file_structure_version = 1;
		uint8_t : 8;
	} PrimaryVolumeDescriptor;

	#pragma pack(pop)

	struct FileSystemEntry {
		std::string identifier;
		bool is_directory;
		size_t first_sector;
		size_t length_bytes;
		std::optional<size_t> parent_first_sector;
	};

	class FileSystem {
		public:

		FileSystem(
			const std::function<void(size_t sector, void* user_data)>& read_user_data
		);

		auto get_entry_at_sector(
			size_t sector
		) -> std::optional<FileSystemEntry>;

		auto get_entry_hierarchy(
			const FileSystemEntry& entry
		) -> const std::vector<FileSystemEntry>&;

		auto get_root_entry(
		) -> const FileSystemEntry&;

		auto get_children(
			const FileSystemEntry& entry
		) -> const std::vector<FileSystemEntry>&;

		protected:

		size_t first_sector;
		std::map<size_t, FileSystemEntry> entries;
		std::map<size_t, std::vector<FileSystemEntry>> children;
		std::map<size_t, std::vector<FileSystemEntry>> hierarchies;
		std::vector<FileSystemEntry> entries_values;
	};
}
