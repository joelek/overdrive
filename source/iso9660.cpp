#include "iso9660.h"

#include <array>
#include <cstring>
#include "idiv.h"

namespace iso9660 {
	namespace internal {
		auto read_file_system_entry(
			const std::function<void(size_t sector, void* user_data)>& read_user_data,
			const FileSystemEntry& fse
		) -> std::vector<uint8_t> {
			auto user_data = std::array<uint8_t, 2048>();
			auto length_sectors = idiv::ceil(fse.length_bytes, 2048);
			auto buffer = std::vector<uint8_t>(fse.length_bytes);
			auto buffer_offset = buffer.data();
			auto buffer_length = fse.length_bytes;
			for (auto sector_index = fse.first_sector; sector_index < fse.first_sector + length_sectors; sector_index += 1) {
				read_user_data(sector_index, user_data.data());
				auto chunk_length = std::min<size_t>(buffer_length, 2048);
				std::memcpy(buffer_offset, user_data.data(), chunk_length);
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
			auto buffer_offset_end = buffer_offset + buffer_length;
			while (buffer_offset < buffer_offset_end) {
				auto& deh = *reinterpret_cast<DirectoryEntryHeader*>(buffer_offset);
				auto record_length = deh.length;
				if (record_length > 0) {
					record_length += deh.extended_record_length;
					auto identifier = std::string(reinterpret_cast<char*>(&deh) + sizeof(DirectoryEntryHeader), deh.identifier_length);
					if (identifier != CURRENT_DIRECTORY_IDENTIFIER && identifier != PARENT_DIRECTORY_IDENTIFIER) {
						auto is_directory = deh.flags.directory == 1;
						auto first_sector = deh.extent_sector_le;
						auto length_bytes = deh.data_length_le;
						auto parent_first_sector = std::optional<size_t>(fse.first_sector);
						fses.push_back({
							identifier,
							is_directory,
							first_sector,
							length_bytes,
							parent_first_sector
						});
					}
				} else {
					auto bytes_parsed = fse.length_bytes - buffer_length;
					auto next_sector = idiv::ceil(bytes_parsed, 2048) * 2048;
					auto skip_bytes = next_sector - bytes_parsed;
					record_length = skip_bytes;
				}
				if (record_length == 0) {
					break;
				}
				buffer_offset += record_length;
				buffer_length -= record_length;
			}
			return fses;
		}

		auto populate_directory_entries(
			const std::function<void(size_t sector, void* user_data)>& read_user_data,
			const FileSystemEntry& fse,
			std::vector<FileSystemEntry> ancestors,
			std::map<size_t, FileSystemEntry>& entries,
			std::map<size_t, std::vector<FileSystemEntry>>& children,
			std::map<size_t, std::vector<FileSystemEntry>>& hierarchies
		) -> void {
			ancestors.push_back(fse);
			if (fse.is_directory) {
				auto fses = internal::read_file_system_entries(read_user_data, fse);
				for (const auto& fse : fses) {
					populate_directory_entries(read_user_data, fse, ancestors, entries, children, hierarchies);
				}
				children[fse.first_sector] = std::move(fses);
			}
			entries[fse.first_sector] = fse;
			hierarchies[fse.first_sector] = std::move(ancestors);
		}
	}

	FileSystem::FileSystem(
		const std::function<void(size_t sector, void* user_data)>& read_user_data
	) {
		auto user_data = std::array<uint8_t, 2048>();
		read_user_data(PRIMARY_VOLUME_DESCRIPTOR_SECTOR, user_data.data());
		auto& pvd = *reinterpret_cast<PrimaryVolumeDescriptor*>(user_data.data());
		auto& deh = pvd.root_directory_entry.header;
		auto identifier = std::string(reinterpret_cast<char*>(&deh) + sizeof(DirectoryEntryHeader), deh.identifier_length);
		auto is_directory = deh.flags.directory == 1;
		auto first_sector = deh.extent_sector_le;
		auto length_bytes = deh.data_length_le;
		auto parent_first_sector = std::optional<size_t>();
		auto root = FileSystemEntry({
			identifier,
			is_directory,
			first_sector,
			length_bytes,
			parent_first_sector
		});
		this->first_sector = first_sector;
		auto ancestors = std::vector<FileSystemEntry>();
		internal::populate_directory_entries(read_user_data, root, ancestors, this->entries, this->children, this->hierarchies);
	}

	auto FileSystem::get_entry_at_sector(
		size_t sector
	) -> std::optional<FileSystemEntry> {
		for (const auto& entry : this->entries) {
			auto& fse = entry.second;
			auto length_sectors = idiv::ceil(fse.length_bytes, 2048);
			if (sector >= fse.first_sector && sector < fse.first_sector + length_sectors) {
				return fse;
			}
		}
		return std::optional<FileSystemEntry>();
	}

	auto FileSystem::get_entry_hierarchy(
		const FileSystemEntry& entry
	) -> const std::vector<FileSystemEntry>& {
		auto iterator = this->hierarchies.find(entry.first_sector);
		if (iterator == this->hierarchies.end()) {
			throw EXIT_FAILURE;
		}
		return iterator->second;
	}

	auto FileSystem::get_root_directory_entry(
	) -> const FileSystemEntry& {
		auto iterator = this->entries.find(this->first_sector);
		if (iterator == this->entries.end()) {
			throw EXIT_FAILURE;
		}
		return iterator->second;
	}

	auto FileSystem::list_directory_entries(
		const FileSystemEntry& entry
	) -> const std::vector<FileSystemEntry>& {
		auto iterator = this->children.find(entry.first_sector);
		if (iterator == this->children.end()) {
			throw EXIT_FAILURE;
		}
		return iterator->second;
	}
}
