#include "odi.h"

namespace commands {
	class ODIptions: public options::Options {
		public:

		protected:
	};

	namespace internal {
	namespace {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ODIptions {
			auto options = ODIptions();
			auto parsers = options::get_default_parsers(options);
			parsers = parser::sort(parsers);
			try {
				parser::parse(arguments, parsers);
				return options;
			} catch (const exceptions::ArgumentException& e) {
				parser::print(parsers);
				throw;
			}
		}

		auto compress_sector(
			copier::ExtractedSector& extracted_sector,
			odi::SectorDataCompressionMethod::type sector_data_method,
			odi::SubchannelsDataCompressionMethod::type subchannels_method
		) -> odi::SectorTableEntry {
			auto sector_table_entry = odi::SectorTableEntry();
			sector_table_entry.readability = extracted_sector.counter == 0 ? odi::Readability::UNREADABLE : odi::Readability::READABLE;
			sector_table_entry.sector_data.compressed_byte_count = cd::SECTOR_LENGTH;
			sector_table_entry.sector_data.compression_method = odi::SectorDataCompressionMethod::NONE;
			sector_table_entry.subchannels_data.compressed_byte_count = cd::SUBCHANNELS_LENGTH;
			sector_table_entry.subchannels_data.compression_method = odi::SubchannelsDataCompressionMethod::NONE;
			try {
				sector_table_entry.sector_data.compressed_byte_count = odi::compress_sector_data(extracted_sector.sector_data, sector_data_method);
				sector_table_entry.sector_data.compression_method = sector_data_method;
			} catch (const exceptions::CompressedSizeExceededUncompressedSizeException& e) {}
			try {
				sector_table_entry.subchannels_data.compressed_byte_count = odi::compress_subchannels_data(extracted_sector.subchannels_data, subchannels_method);
				sector_table_entry.subchannels_data.compression_method = subchannels_method;
			} catch (const exceptions::CompressedSizeExceededUncompressedSizeException& e) {}
			return sector_table_entry;
		}

		auto save_sector_range(
			const drive::Drive& drive,
			si_t first_sector,
			si_t last_sector,
			const ODIptions& options,
			std::FILE* handle,
			const std::string& path
		) -> std::vector<odi::SectorTableEntry> {
			auto extracted_sectors_vector = copier::read_absolute_sector_range(
				drive,
				first_sector,
				last_sector,
				options.min_data_passes,
				options.max_data_passes,
				options.max_data_retries,
				options.min_data_copies,
				options.max_data_copies
			);
			auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, first_sector);
			fprintf(stderr, "%s\n", std::format("Sector range between {} and {} has {} bad sectors!", first_sector, last_sector, bad_sector_indices.size()).c_str());
			auto sector_table_entries = std::vector<odi::SectorTableEntry>(last_sector - first_sector);
			for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
				auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
				auto& extracted_sector = extracted_sectors.at(0);
				auto& sector_table_entry = sector_table_entries.at(sector_index);
				sector_table_entry = compress_sector(extracted_sector, odi::SectorDataCompressionMethod::NONE, odi::SubchannelsDataCompressionMethod::NONE);
				sector_table_entry.compressed_data_absolute_offset = std::ftell(handle);
				if (std::fwrite(extracted_sector.sector_data, sector_table_entry.sector_data.compressed_byte_count, 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				if (std::fwrite(extracted_sector.subchannels_data, sector_table_entry.subchannels_data.compressed_byte_count, 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
			}
			return sector_table_entries;
		}

		auto write_odi(
			const drive::Drive& drive,
			const disc::DiscInfo& disc,
			const ODIptions& options
		) -> void {
			auto points = disc::get_disc_points(disc);
			auto path = path::create_path(options.path)
				.with_extension(".odi")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				auto file_header = odi::FileHeader();
				file_header.header_length = sizeof(odi::FileHeader);
				file_header.sector_table_header_absolute_offset = 0;
				file_header.point_table_header_absolute_offset = 0;
				if (std::fwrite(&file_header, sizeof(file_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				auto sector_table_entries = std::vector<odi::SectorTableEntry>();
				auto absolute_sector_offset = 0 - si_t(disc.sessions.front().lead_in_length_sectors);
				for (auto session_index = size_t(0); session_index < disc.sessions.size(); session_index += 1) {
					auto& session = disc.sessions.at(session_index);
					auto lead_in_sector_table_entries = save_sector_range(drive, absolute_sector_offset, absolute_sector_offset + session.lead_in_length_sectors, options, handle, path);
					vector::append(sector_table_entries, lead_in_sector_table_entries);
					absolute_sector_offset += session.lead_in_length_sectors;
					auto pregap_sector_table_entries = save_sector_range(drive, absolute_sector_offset, absolute_sector_offset + session.pregap_sectors, options, handle, path);
					vector::append(sector_table_entries, pregap_sector_table_entries);
					absolute_sector_offset += session.pregap_sectors;
					for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
						auto& track = session.tracks.at(track_index);
						auto extracted_sectors_vector = copier::read_track(drive, track, options);
						auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
						copier::log_bad_sector_indices(drive, track, bad_sector_indices);
						auto compressed_byte_count = size_t(0);
						auto track_sector_table_entries = std::vector<odi::SectorTableEntry>(track.length_sectors);
						for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
							auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
							auto& extracted_sector = extracted_sectors.at(0);
							auto& sector_table_entry = track_sector_table_entries.at(sector_index);
							auto sector_data_method = track.type == disc::TrackType::AUDIO_2_CHANNELS ? odi::SectorDataCompressionMethod::LOSSLESS_STEREO_AUDIO : odi::SectorDataCompressionMethod::NONE;
							auto subchannels_data_method = odi::SubchannelsDataCompressionMethod::NONE;
							sector_table_entry = compress_sector(extracted_sector, sector_data_method, subchannels_data_method);
						}
						for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
							auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
							auto& extracted_sector = extracted_sectors.at(0);
							auto& sector_table_entry = track_sector_table_entries.at(sector_index);
							sector_table_entry.compressed_data_absolute_offset = std::ftell(handle);
							if (std::fwrite(extracted_sector.sector_data, sector_table_entry.sector_data.compressed_byte_count, 1, handle) != 1) {
								OVERDRIVE_THROW(exceptions::IOWriteException(path));
							}
							if (std::fwrite(extracted_sector.subchannels_data, sector_table_entry.subchannels_data.compressed_byte_count, 1, handle) != 1) {
								OVERDRIVE_THROW(exceptions::IOWriteException(path));
							}
							compressed_byte_count += sector_table_entry.sector_data.compressed_byte_count;
						}
						vector::append(sector_table_entries, track_sector_table_entries);
						absolute_sector_offset += track.length_sectors;
						auto compression_rate = float(compressed_byte_count) / (extracted_sectors_vector.size() * cd::SECTOR_LENGTH);
						fprintf(stderr, "%s\n", std::format("Saved track {} with a compression rate of {:.2f}", track.number, compression_rate).c_str());
					}
					auto lead_out_sector_table_entries = save_sector_range(drive, absolute_sector_offset, absolute_sector_offset + session.lead_out_length_sectors, options, handle, path);
					vector::append(sector_table_entries, lead_out_sector_table_entries);
					absolute_sector_offset += session.lead_out_length_sectors;
				}
				auto point_table_entries = std::vector<odi::PointTableEntry>();
				for (auto point_index = size_t(0); point_index < points.size(); point_index += 1) {
					auto& point = points.at(point_index);
					auto point_table_entry = odi::PointTableEntry();
					*reinterpret_cast<cdb::ReadTOCResponseFullTOCEntry*>(&point_table_entry.entry) = point.entry;
					point_table_entries.push_back(point_table_entry);
				}
				std::fseek(handle, idiv::ceil(std::ftell(handle), 16) * 16, SEEK_SET);
				auto sector_table_header = odi::SectorTableHeader();
				sector_table_header.entry_count = sector_table_entries.size();
				sector_table_header.entry_length = sizeof(odi::SectorTableEntry);
				sector_table_header.header_length = sizeof(odi::SectorTableHeader);
				file_header.sector_table_header_absolute_offset = std::ftell(handle);
				if (std::fwrite(&sector_table_header, sizeof(sector_table_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				for (auto sector_table_index = size_t(0); sector_table_index < sector_table_header.entry_count; sector_table_index += 1) {
					auto& sector_table_entry = sector_table_entries.at(sector_table_index);
					if (std::fwrite(&sector_table_entry, sizeof(sector_table_entry), 1, handle) != 1) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
				}
				std::fseek(handle, idiv::ceil(std::ftell(handle), 16) * 16, SEEK_SET);
				auto point_table_header = odi::PointTableHeader();
				point_table_header.entry_count = point_table_entries.size();
				point_table_header.entry_length = sizeof(odi::PointTableEntry);
				point_table_header.header_length = sizeof(odi::PointTableHeader);
				file_header.point_table_header_absolute_offset = std::ftell(handle);
				if (std::fwrite(&point_table_header, sizeof(point_table_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				for (auto point_table_index = size_t(0); point_table_index < point_table_header.entry_count; point_table_index += 1) {
					auto& point_table_entry = point_table_entries.at(point_table_index);
					if (std::fwrite(&point_table_entry, sizeof(point_table_entry), 1, handle) != 1) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
				}
				std::fseek(handle, 0, SEEK_SET);
				if (std::fwrite(&file_header, sizeof(file_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
			}  catch (...) {
				copier::close_handle(handle);
				throw;
			}
			copier::close_handle(handle);
		}
	}
	}

	auto odi(
		const std::vector<std::string>& arguments
	) -> void {
		auto options = internal::parse_options(arguments);
		auto detail = detail::create_detail();
		auto drive_handle = detail.get_handle(options.drive);
		auto drive = drive::create_drive(drive_handle, detail.ioctl);
		auto drive_info = drive.read_drive_info();
		drive_info.print();
		auto disc_info = drive.read_disc_info();
		disc_info.print();
		if (!options.read_correction) {
			options.read_correction = drive_info.read_offset_correction;
		}
		internal::write_odi(drive, disc_info, options);
	};
}
