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

		auto assert_image_compatibility(
			const std::vector<disc::TrackInfo>& tracks
		) -> void {
			(void)tracks;
		}

		auto write_odi(
			const drive::Drive& drive,
			const disc::DiscInfo& disc,
			const ODIptions& options
		) -> void {
			auto tracks = disc::get_disc_tracks(disc);
			auto points = disc::get_disc_points(disc);
			auto path = path::create_path(options.path)
				.with_extension(".odi")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				auto file_header = odi::FileHeader();
				file_header.sector_table_header_absolute_offset = 0; // TODO
				file_header.point_table_header_absolute_offset = 0; // TODO
				if (std::fwrite(&file_header, sizeof(file_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}


				auto sector_table_entries = std::vector<odi::SectorTableEntry>();
				sector_table_entries.reserve(0); // TODO
				for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
					auto& track = tracks.at(track_index);
					auto extracted_sectors_vector = copier::read_track(drive, track, options);
					auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
					copier::log_bad_sector_indices(drive, track, bad_sector_indices);
					for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
						auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
						auto& extracted_sector = extracted_sectors.at(0);
						auto& sector_table_entry = sector_table_entries.at(sector_index + track.first_sector_absolute);
						sector_table_entry.compressed_data_absolute_offset = std::ftell(handle);
						if (std::fwrite(extracted_sector.sector_data, sizeof(extracted_sector.sector_data), 1, handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(path));
						}
						if (std::fwrite(extracted_sector.subchannels_data, sizeof(extracted_sector.subchannels_data), 1, handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(path));
						}

					}
				}


				file_header.sector_table_header_absolute_offset = std::ftell(handle);
				auto sector_table_header = odi::SectorTableHeader();
				sector_table_header.entry_count = cd::RELATIVE_SECTOR_OFFSET + 0; // TODO
				sector_table_header.entry_length = sizeof(odi::SectorTableEntry);
				if (std::fwrite(&sector_table_header, sizeof(sector_table_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				for (auto sector_index = size_t(0); sector_index < sector_table_header.entry_count; sector_index += 1) {
					auto sector_table_entry = odi::SectorTableEntry();
					sector_table_entry.compressed_data_absolute_offset = 0; // TODO
					sector_table_entry.compressed_byte_count = sizeof(odi::UncompressedSector);
					sector_table_entry.compression_method = odi::CompressionMethod::NONE;
					sector_table_entry.readability = odi::Readability::UNREADABLE; // TODO
					if (std::fwrite(&sector_table_entry, sizeof(sector_table_entry), 1, handle) != 1) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
				}





				file_header.point_table_header_absolute_offset = std::ftell(handle);
				auto point_table_header = odi::PointTableHeader();
				point_table_header.entry_count = points.size();
				point_table_header.entry_length = sizeof(odi::PointTableEntry);
				if (std::fwrite(&sector_table_header, sizeof(sector_table_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				for (auto point_index = size_t(0); point_index < point_table_header.entry_count; point_index += 1) {
					auto& point = points.at(point_index);
					auto point_table_entry = odi::PointTableEntry();
					*reinterpret_cast<cdb::ReadTOCResponseFullTOCEntry*>(&point_table_entry.entry) = point.entry;
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
