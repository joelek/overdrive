#include "mds.h"

#include <cstdio>
#include <format>

namespace commands {
	class MDSOptions: public options::Options {
		public:

		bool_t save_data_subchannels;

		protected:
	};

	namespace internal {
	namespace {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> MDSOptions {
			auto options = MDSOptions();
			auto parsers = options::get_default_parsers(options);
			parsers.push_back(parser::Parser({
				"save-data-subchannels",
				"Specify whether to save subchannel data for data tracks.",
				std::regex("^(true|false)$"),
				"boolean",
				false,
				std::optional<std::string>("false"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.save_data_subchannels = matches.at(0) == "true";
				}
			}));
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

		auto write_mdf(
			const drive::Drive& drive,
			const std::vector<disc::TrackInfo>& tracks,
			const MDSOptions& options
		) -> std::vector<size_t> {
			auto result = std::vector<size_t>();
			auto path = path::create_path(options.path)
				.with_extension(".mdf")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
					auto& track = tracks.at(track_index);
					auto extracted_sectors_vector = copier::read_track(drive, track, options);
					auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
					copier::log_bad_sector_indices(drive, track, bad_sector_indices);
					copier::append_sector_data(extracted_sectors_vector, path, 0, cd::SECTOR_LENGTH, handle); // TODO: Write subchannels.
					vector::append<size_t>(result, bad_sector_indices);
				}
			}  catch (...) {
				copier::close_handle(handle);
				throw;
			}
			copier::close_handle(handle);
			return result;
		}

		auto write_mds(
			const disc::DiscInfo& disc,
			const MDSOptions& options,
			const std::vector<size_t>& bad_sector_indices
		) -> void {
			auto path = path::create_path(options.path)
				.with_extension(".mds")
				.create_directories();
			auto handle = copier::open_handle(path);
			auto tracks = disc::get_disc_tracks(disc);
			auto points = disc::get_disc_points(disc);
			auto absolute_offset_to_track_table_entry = sizeof(mds::FormatHeader) + disc.sessions.size() * sizeof(mds::SessionHeader) + points.size() * sizeof(mds::Entry) + sizeof(mds::TrackTableHeader);
			auto absolute_offset_to_file_table_header = absolute_offset_to_track_table_entry + tracks.size() * sizeof(mds::TrackTableEntry);
			auto absolute_offset_to_file_table_entry = absolute_offset_to_file_table_header + sizeof(mds::FileTableHeader);
			auto absolute_offset_to_footer = absolute_offset_to_file_table_entry + sizeof(mds::FileTableEntry);
			auto absolute_offset_to_bad_sectors_table_header = absolute_offset_to_footer + sizeof(mds::Footer);
			auto format_header = mds::FormatHeader();
			if (bad_sector_indices.size() > 0) {
				format_header.absolute_offset_to_footer = absolute_offset_to_footer;
			}
			if (std::fwrite(&format_header, sizeof(format_header), 1, handle) != 1) {
				OVERDRIVE_THROW(exceptions::IOWriteException(path));
			}
			auto absolute_offset_to_entry_table = sizeof(mds::FormatHeader);
			auto first_sector_on_disc = 0;
			auto mdf_byte_offset = 0;
			auto track_index = 0;
			for (auto session_index = size_t(0); session_index < disc.sessions.size(); session_index += 1) {
				absolute_offset_to_entry_table += sizeof(mds::SessionHeader);
				auto& session = disc.sessions.at(session_index);
				auto session_header = mds::SessionHeader();
				session_header.pregap_correction = 0 - static_cast<si32_t>(cd::PHYSICAL_SECTOR_OFFSET);
				session_header.sectors_on_disc = session.length_sectors;
				session_header.session_number = session.number;
				session_header.entry_count = session.points.size();
				session_header.entry_count_type_a = session.points.size() - session.tracks.size();
				session_header.first_track = session.tracks.front().number;
				session_header.last_track = session.tracks.back().number;
				session_header.absolute_offset_to_entry_table = absolute_offset_to_entry_table;
				if (std::fwrite(&session_header, sizeof(session_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				absolute_offset_to_entry_table += session.points.size() * sizeof(mds::Entry);
				for (auto point_index = size_t(0); point_index < session.points.size(); point_index += 1) {
					auto& point = session.points.at(point_index);
					if (point.entry.adr == 1 && point.entry.point >= 1 && point.entry.point <= 99) {
						auto entry = mds::EntryTypeB();
						entry.track_mode = mds::get_track_mode(track.type);
						entry.track_mode_flags = entry.track_mode == mds::TrackMode::MODE2_FORM1 || entry.track_mode == mds::TrackMode::MODE2_FORM2 ? mds::TrackModeFlags::UNKNOWN_E : mds::TrackModeFlags::UNKNOWN_A;
						entry.entry = point.entry;
						// Must be written after entry since the two fields overlap.
						entry.subchannel_mode = options.save_data_subchannels ? mds::SubchannelMode::INTERLEAVED_96 : mds::SubchannelMode::NONE;
						entry.absolute_offset_to_track_table_entry = absolute_offset_to_track_table_entry + track_index * sizeof(mds::TrackTableEntry);
						entry.sector_length = options.save_data_subchannels ? cd::SECTOR_LENGTH + cd::SUBCHANNELS_LENGTH : cd::SECTOR_LENGTH;
						entry.unknown_a = 2;
						entry.first_sector_on_disc = first_sector_on_disc;
						entry.mdf_byte_offset = mdf_byte_offset;
						entry.unknown_d = 1;
						entry.absolute_offset_to_file_table_header = absolute_offset_to_file_table_header;
						if (std::fwrite(&entry, sizeof(entry), 1, handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(path));
						}
						first_sector_on_disc += track.length_sectors;
						mdf_byte_offset += track.length_sectors * entry.sector_length;
						track_index += 1;
					} else {
						auto entry = mds::EntryTypeA();
						entry.track_mode = mds::TrackMode::NONE;
						entry.track_mode_flags = mds::TrackModeFlags::UNKNOWN_0;
						entry.entry = point.entry;
						// Must be written after entry since the two fields overlap.
						entry.subchannel_mode = mds::SubchannelMode::NONE;
						if (std::fwrite(&entry, sizeof(entry), 1, handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(path));
						}
					}
				}
			}
			auto track_table_header = mds::TrackTableHeader();
			if (std::fwrite(&track_table_header, sizeof(track_table_header), 1, handle) != 1) {
				OVERDRIVE_THROW(exceptions::IOWriteException(path));
			}
			for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
				auto& track = tracks.at(track_index);
				auto track_table_entry = mds::TrackTableEntry();
				track_table_entry.pregap_sectors = track_index == 0 ? cd::PHYSICAL_SECTOR_OFFSET : 0;
				track_table_entry.length_sectors = track.length_sectors;
				if (std::fwrite(&track_table_entry, sizeof(track_table_entry), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
			}
			auto file_table_header = mds::FileTableHeader();
			file_table_header.absolute_offset_to_file_table_entry = absolute_offset_to_file_table_entry;
			if (std::fwrite(&file_table_header, sizeof(file_table_header), 1, handle) != 1) {
				OVERDRIVE_THROW(exceptions::IOWriteException(path));
			}
			auto file_table_entry = mds::FileTableEntry();
			if (std::fwrite(&file_table_entry, sizeof(file_table_entry), 1, handle) != 1) {
				OVERDRIVE_THROW(exceptions::IOWriteException(path));
			}
			if (bad_sector_indices.size() > 0) {
				auto footer = mds::Footer();
				footer.absolute_offset_to_bad_sectors_table_header = absolute_offset_to_bad_sectors_table_header;
				if (std::fwrite(&footer, sizeof(footer), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				auto bad_sector_table_header = mds::BadSectorTableHeader();
				bad_sector_table_header.bad_sector_count = bad_sector_indices.size();
				if (std::fwrite(&bad_sector_table_header, sizeof(bad_sector_table_header), 1, handle) != 1) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				for (auto bad_sector_index : bad_sector_indices) {
					auto bad_sector_table_entry = mds::BadSectorTableEntry();
					bad_sector_table_entry.bad_sector_index = bad_sector_index;
					if (std::fwrite(&bad_sector_table_entry, sizeof(bad_sector_table_entry), 1, handle) != 1) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
				}
			}
		}
	}
	}

	auto mds(
		const std::vector<std::string>& arguments,
		const command::Detail& detail
	) -> void {
		auto options = internal::parse_options(arguments);
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
		auto bad_sector_indices = internal::write_mdf(drive, tracks, options);
		internal::write_mds(disc_info, options, bad_sector_indices);
	};
}
