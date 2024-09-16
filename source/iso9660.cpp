#include "iso9660.h"

#include <cstring>
#include "idiv.h"

namespace iso9660 {
	namespace internal {
		auto read_file_system_entry(
			const std::function<void(size_t sector, void* user_data)>& read_user_data,
			const FileSystemEntry& fse
		) -> std::vector<uint8_t> {
			uint8_t user_data[2048];
			auto length_sectors = idiv::ceil(fse.length_bytes, 2048);
			auto buffer = std::vector<uint8_t>(fse.length_bytes);
			auto buffer_offset = buffer.data();
			auto buffer_length = fse.length_bytes;
			for (auto sector_index = fse.first_sector; sector_index < fse.first_sector + length_sectors; sector_index += 1) {
				read_user_data(sector_index, user_data);
				auto chunk_length = std::min<size_t>(buffer_length, 2048);
				std::memcpy(buffer_offset, user_data, chunk_length);
				buffer_offset += chunk_length;
				buffer_length -= chunk_length;
			}
			return buffer;
		}

		auto read_file_system_entries(
			const std::function<void(size_t sector, void* user_data)>& read_user_data,
			const FileSystemEntry& fse
		) -> std::vector<FileSystemEntry> {
			auto fses = std::vector<FileSystemEntry>();
			auto buffer = read_file_system_entry(read_user_data, fse);
			auto buffer_offset = buffer.data();
			auto buffer_length = fse.length_bytes;
			while (buffer_offset < buffer_offset + fse.length_bytes) {
				auto& deh = *reinterpret_cast<DirectoryEntryHeader*>(buffer_offset);
				auto record_length = deh.length;
				if (record_length == 0) {
					break;
				}
				record_length += deh.extended_record_length;
				auto identifier = std::string(reinterpret_cast<char*>(&deh) + sizeof(DirectoryEntryHeader), deh.identifier_length);
				if (identifier != CURRENT_DIRECTORY_IDENTIFIER && identifier != PARENT_DIRECTORY_IDENTIFIER) {
					auto is_directory = deh.flags.directory == 1;
					auto first_sector = deh.extent_sector_le;
					auto length_bytes = deh.data_length_le;
					fses.push_back({
						identifier,
						is_directory,
						first_sector,
						length_bytes
					});
				}
				buffer_offset += record_length;
				buffer_length -= record_length;
			}
			return fses;
		}

		auto populate_directory_entries(
			const std::function<void(size_t sector, void* user_data)>& read_user_data,
			const FileSystemEntry& fse,
			std::map<size_t, FileSystemEntry>& entries,
			std::map<size_t, std::vector<size_t>>& children
		) -> void {
			if (fse.is_directory) {
				auto fses = internal::read_file_system_entries(read_user_data, fse);
				auto first_sectors = std::vector<size_t>();
				for (const auto& fse : fses) {
					populate_directory_entries(read_user_data, fse, entries, children);
					first_sectors.push_back(fse.first_sector);
				}
				children[fse.first_sector] = std::move(first_sectors);
			}
			entries[fse.first_sector] = fse;
		}
	}

	FileSystem::FileSystem(
		const std::function<void(size_t sector, void* user_data)>& read_user_data
	) {
		uint8_t user_data[2048];
		read_user_data(PRIMARY_VOLUME_DESCRIPTOR_SECTOR, user_data);
		auto& pvd = *reinterpret_cast<PrimaryVolumeDescriptor*>(user_data);
		auto& deh = pvd.root_directory_entry.header;
		auto identifier = std::string(reinterpret_cast<char*>(&deh) + sizeof(DirectoryEntryHeader), deh.identifier_length);
		auto is_directory = deh.flags.directory == 1;
		auto first_sector = deh.extent_sector_le;
		auto length_bytes = deh.data_length_le;
		auto root = FileSystemEntry({
			identifier,
			is_directory,
			first_sector,
			length_bytes
		});
		this->first_sector = first_sector;
		internal::populate_directory_entries(read_user_data, root, this->entries, this->children);




		for (const auto& e : this->entries) {
			fprintf(stderr, "%i %s\n", e.first, e.second.identifier.c_str());
		}
		for (const auto& e : this->children) {
			fprintf(stderr, "%i %i\n", e.first, e.second.size());
		}
	}
/*
	auto FileSystem::list_directory_entries() -> const std::vector<FileSystemEntry>& {

	}

	auto FileSystem::list_directory_entries(const FileSystemEntry& entry) -> const std::vector<FileSystemEntry>& {

	} */
}
