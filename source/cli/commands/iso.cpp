#include "iso.h"

#include <format>

namespace commands {
	class ISOOptions: public options::Options {
		public:

		protected:
	};

	namespace internal {
	namespace {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto options = ISOOptions();
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
	}
	}

	auto iso(
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
		auto tracks = disc::get_disc_tracks(disc_info, options.track_numbers);
		internal::assert_image_compatibility(tracks);
		for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
			auto& track = tracks.at(track_index);
			auto extracted_sectors_vector = copier::read_track(drive, track, options);
			auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
			copier::log_bad_sector_indices(drive, track, bad_sector_indices);
			if (disc::is_data_track(track.type)) {
				auto user_data_offset = disc::get_user_data_offset(track.type);
				auto user_data_length = disc::get_user_data_length(track.type);
				auto path = path::create_path(options.path).with_extension(std::format(".{:0>2}.iso", track.number));
				path.create_directories();
				copier::write_sector_data_to_file(extracted_sectors_vector, path, user_data_offset, user_data_length);
			} else {
				OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
			}
		}
	};
}
