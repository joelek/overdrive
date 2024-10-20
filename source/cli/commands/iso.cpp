#include "iso.h"

#include <algorithm>
#include <format>
#include <optional>
#include <regex>

namespace commands {
	class ISOOptions: public options::Options {
		public:

		std::optional<std::set<size_t>> track_numbers;

		protected:
	};

	namespace internal {
	namespace {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto options = ISOOptions();
			auto parsers = options::get_default_parsers(options);
			parsers.push_back(parser::Parser({
				"track-numbers",
				{},
				"Specify which track numbers to save.",
				std::regex("^((?:[1-9]|[1-9][0-9])(?:[-](?:[1-9]|[1-9][0-9]))?)$"),
				"integer",
				false,
				std::optional<std::string>(),
				0,
				99,
				[&](const std::vector<std::string>& matches) -> void {
					auto track_numbers = std::set<size_t>();
					for (auto& match : matches) {
						auto parts = string::split(match, "-");
						if (parts.size() == 1) {
							track_numbers.insert(std::atoi(parts.at(0).c_str()));
						} else {
							auto one = std::atoi(parts.at(0).c_str());
							auto two = std::atoi(parts.at(1).c_str());
							if (two < one) {
								std::swap(one, two);
							}
							for (auto track_number = one; track_number <= two; track_number += 1) {
								track_numbers.insert(track_number);
							}
						}
					}
					options.track_numbers = track_numbers;
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
			for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
				auto& track = tracks.at(track_index);
				if (disc::is_data_track(track.type)) {
					auto user_data_length = disc::get_user_data_length(track.type);
					if (user_data_length != iso9660::USER_DATA_SIZE) {
						OVERDRIVE_THROW(exceptions::InvalidValueException("user data length", user_data_length, iso9660::USER_DATA_SIZE, iso9660::USER_DATA_SIZE));
					}
				} else {
					OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
				}
			}
		}

		auto write_iso(
			const drive::Drive& drive,
			const std::vector<disc::TrackInfo>& tracks,
			const ISOOptions& options
		) -> void {
			for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
				auto& track = tracks.at(track_index);
				auto extracted_sectors_vector = copier::read_track(drive, track, options);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				copier::log_bad_sector_indices(drive, track, bad_sector_indices);
				if (disc::is_data_track(track.type)) {
					auto user_data_offset = disc::get_user_data_offset(track.type);
					auto user_data_length = disc::get_user_data_length(track.type);
					auto path = path::create_path(options.path)
						.with_extension(std::format(".{:0>2}.iso", track.number))
						.create_directories();
					copier::write_sector_data_to_file(extracted_sectors_vector, path, user_data_offset, user_data_length, false);
				} else {
					OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
				}
			}
		}
	}
	}

	auto iso(
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
		auto tracks = disc::get_disc_tracks(disc_info, options.track_numbers);
		internal::assert_image_compatibility(tracks);
		internal::write_iso(drive, tracks, options);
	};
}
