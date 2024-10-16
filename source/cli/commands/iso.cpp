#include "iso.h"

#include <format>

namespace commands {
	class ISOOptions: public options::Options {
		public:

		protected:
	};

	namespace internal {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto options = ISOOptions();
			auto parsers = options::get_default_parsers(options);
			parser::sort(parsers);
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
					// OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
				}
			}
		}
	}

	auto iso(
		const std::vector<std::string>& arguments,
		const Detail& detail
	) -> void {
		auto options = internal::parse_options(arguments);
		auto handle = detail.get_handle(options.drive);
		auto drive = drive::create_drive(handle, detail.ioctl);
		auto drive_info = drive.read_drive_info();
		drive_info.print();
		auto disc_info = drive.read_disc_info();
		disc_info.print();
		auto read_correction_samples = options.read_correction ? options.read_correction.value() : drive_info.read_offset_correction ? drive_info.read_offset_correction.value() : 0;
		auto tracks = disc::get_disc_tracks(disc_info, options.track_numbers);
		internal::assert_image_compatibility(tracks);
		for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
			auto& track = tracks.at(track_index);
			fprintf(stderr, "%s\n", std::format("Extracting track number {} containing {} sectors from {} to {}", track.number, track.length_sectors, track.first_sector_absolute, track.last_sector_absolute).c_str());
			if (disc::is_data_track(track.type)) {
				auto extracted_sectors_vector = copier::read_absolute_sector_range(
					drive,
					track.first_sector_absolute,
					track.last_sector_absolute,
					options.min_data_passes,
					options.max_data_passes,
					options.max_data_retries,
					options.min_data_copies,
					options.max_data_copies
				);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				copier::log_bad_sector_indices(drive, track, bad_sector_indices);
				auto user_data_offset = disc::get_user_data_offset(track.type);
				auto user_data_length = disc::get_user_data_length(track.type);
				auto iso_path = copier::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.iso", track.number));
				copier::write_sector_data_to_file(extracted_sectors_vector, iso_path, user_data_offset, user_data_length);
			} else {
				auto extracted_sectors_vector = copier::read_absolute_sector_range_with_correction(
					drive,
					track.first_sector_absolute,
					track.last_sector_absolute,
					options.min_audio_passes,
					options.max_audio_passes,
					options.max_audio_retries,
					options.min_audio_copies,
					options.max_audio_copies,
					read_correction_samples
				);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				copier::log_bad_sector_indices(drive, track, bad_sector_indices);
				auto bin_path = copier::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.bin", track.number));
				copier::write_sector_data_to_file(extracted_sectors_vector, bin_path, 0, cd::SECTOR_LENGTH);
			}
		}
	};
}
