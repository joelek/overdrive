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
			const std::vector<disc::TrackInfo>& tracks,
			const ODIptions& options
		) -> void {
			auto path = path::create_path(options.path)
				.with_extension(".odi")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				auto file_header = odi::FileHeader();
				file_header.sector_table_header_absolute_offset = sizeof(file_header);
				if (std::fwrite(&file_header, sizeof(file_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				auto sector_table_header = odi::SectorTableHeader();
				sector_table_header.entry_count = 0; // TODO
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





				for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
					auto& track = tracks.at(track_index);
					auto extracted_sectors_vector = copier::read_track(drive, track, options);
					auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
					copier::log_bad_sector_indices(drive, track, bad_sector_indices);
					if (disc::is_data_track(track.type)) {
						copier::append_sector_data(extracted_sectors_vector, path, 0, cd::SECTOR_LENGTH, handle, true);
					} else {
						copier::append_sector_data(extracted_sectors_vector, path, 0, cd::SECTOR_LENGTH, handle, true);
					}
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
		auto tracks = disc::get_disc_tracks(disc_info);
		internal::assert_image_compatibility(tracks);
		internal::write_odi(drive, tracks, options);
	};
}
