#include "iso9660.h"

#include <algorithm>
#include <array>
#include <cstring>
#include "exceptions.h"
#include "idiv.h"

namespace overdrive {
namespace iso9660 {
	namespace internal {
		auto read_file_system_entry(
			const std::function<void(size_t sector, void* user_data)>& read_user_data,
			const FileSystemEntry& fse
		) -> std::vector<byte_t> {
			auto user_data = std::array<byte_t, USER_DATA_SIZE>();
			auto length_sectors = idiv::ceil(fse.length_bytes, USER_DATA_SIZE);
			auto buffer = std::vector<byte_t>(fse.length_bytes);
			auto buffer_offset = buffer.data();
			auto buffer_length = fse.length_bytes;
			for (auto sector_index = fse.first_sector; sector_index < fse.first_sector + length_sectors; sector_index += 1) {
				read_user_data(sector_index, user_data.data());
				auto chunk_length = std::min<size_t>(buffer_length, USER_DATA_SIZE);
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
					auto next_sector = idiv::ceil(bytes_parsed, USER_DATA_SIZE) * USER_DATA_SIZE;
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
			std::vector<FileSystemEntry>& entries,
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
			entries.push_back(fse);
			hierarchies[fse.first_sector] = std::move(ancestors);
		}

		auto bisect_entries(
			const std::vector<FileSystemEntry>& entries,
			size_t lower_index_inclusive,
			size_t upper_index_exclusive,
			size_t sector
		) -> std::optional<FileSystemEntry> {
			if (lower_index_inclusive + 1 == upper_index_exclusive) {
				auto& fse = entries.at(lower_index_inclusive);
				auto length_sectors = idiv::ceil(fse.length_bytes, USER_DATA_SIZE);
				if (sector >= fse.first_sector && sector < fse.first_sector + length_sectors) {
					return fse;
				}
			} else if (lower_index_inclusive + 1 < upper_index_exclusive) {
				auto pivot_index = (lower_index_inclusive + upper_index_exclusive) / 2;
				auto& fse = entries.at(pivot_index);
				if (sector < fse.first_sector) {
					return bisect_entries(entries, lower_index_inclusive, pivot_index, sector);
				} else {
					return bisect_entries(entries, pivot_index, upper_index_exclusive, sector);
				}
			}
			return std::optional<FileSystemEntry>();
		}
	}

	FileSystem::FileSystem(
		const std::function<void(size_t sector, void* user_data)>& read_user_data
	) {
		auto user_data = std::array<byte_t, USER_DATA_SIZE>();
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
		auto ancestors = std::vector<FileSystemEntry>();
		internal::populate_directory_entries(read_user_data, root, ancestors, this->entries, this->children, this->hierarchies);
		this->root = root;
		// Sort in increasing order.
		std::sort(this->entries.begin(), this->entries.end(), [](const FileSystemEntry& one, const FileSystemEntry& two) -> bool_t {
			return one.first_sector < two.first_sector;
		});
	}

	auto FileSystem::get_children(
		const FileSystemEntry& entry
	) const -> const std::vector<FileSystemEntry>& {
		auto iterator = this->children.find(entry.first_sector);
		if (iterator == this->children.end()) {
			OVERDRIVE_THROW(exceptions::MissingValueException("children"));
		}
		return iterator->second;
	}

	auto FileSystem::get_entry(
		size_t sector
	) const -> std::optional<FileSystemEntry> {
		return internal::bisect_entries(this->entries, 0, this->entries.size(), sector);
	}

	auto FileSystem::get_hierarchy(
		const FileSystemEntry& entry
	) const -> const std::vector<FileSystemEntry>& {
		auto iterator = this->hierarchies.find(entry.first_sector);
		if (iterator == this->hierarchies.end()) {
			OVERDRIVE_THROW(exceptions::MissingValueException("hierarchy"));
		}
		return iterator->second;
	}

	auto FileSystem::get_root(
	) const -> const FileSystemEntry& {
		return this->root;
	}

	auto FileSystem::get_path(
		size_t sector
	) const -> std::optional<std::vector<std::string>> {
		auto entry = this->get_entry(sector);
		if (entry) {
			auto paths = std::vector<std::string>();
			auto& entries = this->get_hierarchy(entry.value());
			for (auto& entry : entries) {
				if (entry.identifier == CURRENT_DIRECTORY_IDENTIFIER) {
					continue;
				}
				if (entry.identifier == PARENT_DIRECTORY_IDENTIFIER) {
					continue;
				}
				paths.push_back(entry.identifier);
			}
			return paths;
		}
		return std::optional<std::vector<std::string>>();
	}
}
}
