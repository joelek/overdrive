#include "iso.h"

#include <cstring>
#include <filesystem>
#include <format>
#include <optional>
#include <regex>
#include <string>

namespace commands {
	class ISOOptions {
		public:

		std::string drive;
		std::optional<si_t> read_offset_correction;
		std::optional<ui_t> tracks;
		std::string path;

		protected:
	};

	namespace internal {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto drive = std::optional<std::string>();
			auto read_offset_correction = std::optional<si_t>();
			auto tracks = std::optional<ui_t>();
			auto path = std::string("image.iso");
			for (auto argument_index = size_t(2); argument_index < arguments.size(); argument_index += 1) {
				auto& argument = arguments[argument_index];
				if (false) {
				} else if (argument.find("--drive=") == 0) {
					auto value = argument.substr(sizeof("--drive=") - 1);
					auto format = std::string("^([A-Z])[:]?$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						drive = matches[1];
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("drive", format));
					}
				} else if (argument.find("--read-offset-correction=") == 0) {
					auto value = argument.substr(sizeof("--read-offset-correction=") - 1);
					auto format = std::string("^([+-]?(?:[0-9]|[1-9][0-9]+))$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						read_offset_correction = std::atoi(matches[1].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("read-offset-correction", format));
					}
				} else if (argument.find("--tracks=") == 0) {
					auto value = argument.substr(sizeof("--tracks=") - 1);
					auto format = std::string("^([1-9]|[1-9][0-9])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						tracks = std::atoi(matches[1].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("tracks", format));
					}
				} else if (argument.find("--path=") == 0) {
					auto value = argument.substr(sizeof("--path=") - 1);
					auto format = std::string("^(.+)$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						path = matches[1];
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("path", format));
					}
				} else {
					OVERDRIVE_THROW(exceptions::UnknownArgumentException(argument));
				}
			}
			if (!drive) {
				OVERDRIVE_THROW(exceptions::MissingArgumentException("drive"));
			}
			return {
				drive.value(),
				read_offset_correction,
				tracks,
				path
			};
		}

		auto check_disc(
			const disc::DiscInfo& disc_info
		) -> void {
			if (disc_info.sessions.size() != 1) {
				OVERDRIVE_THROW(exceptions::InvalidValueException("sessions", disc_info.sessions.size(), 1, 1));
			}
			for (auto session_index = size_t(0); session_index < disc_info.sessions.size(); session_index += 1) {
				auto& session = disc_info.sessions.at(session_index);
				if (session.tracks.size() != 1) {
					OVERDRIVE_THROW(exceptions::InvalidValueException("tracks", session.tracks.size(), 1, 1));
				}
				for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
					auto& track = session.tracks.at(track_index);
					if (disc::is_data_track(track.type)) {
						auto user_data_size = disc::get_user_data_length(track.type);
						if (user_data_size != iso9660::USER_DATA_SIZE) {
							OVERDRIVE_THROW(exceptions::InvalidValueException("user data size", user_data_size, iso9660::USER_DATA_SIZE, iso9660::USER_DATA_SIZE));
						}
					} else {
						OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
					}
				}
			}
		}

		auto copy_data_track(
			const drive::Drive& drive,
			const disc::TrackInfo& track_info
		) -> void {
			auto sector = cdb::ReadCDResponseDataA();
			auto empty_sector = cdb::ReadCDResponseDataA();
			auto user_data_offset = disc::get_user_data_offset(track_info.type);
			auto fs = iso9660::FileSystem([&](size_t sector_index, void* user_data) -> void {
				drive.read_sector(sector_index, &sector.sector_data, nullptr, nullptr);
				std::memcpy(user_data, sector.sector_data + user_data_offset, iso9660::USER_DATA_SIZE);
			});
			fprintf(stderr, "%s\n", std::format("Extracting {} sectors from {} to {}", track_info.length_sectors, track_info.first_sector_absolute, track_info.last_sector_absolute - 1).c_str());
			for (auto sector_index = track_info.first_sector_absolute; sector_index < track_info.last_sector_absolute; sector_index += 1) {
				try {
					drive.read_sector(sector_index, &sector.sector_data, &sector.subchannels_data, &sector.c2_data);
					// TODO: Write and check for error.
				} catch (...) {
					// bad_sector_numbers.push_back(sector_index);
					fprintf(stderr, "%s\n", std::format("Error reading sector {}!", sector_index).c_str());
					auto path = fs.get_path(sector_index);
					if (path) {
						fprintf(stderr, "%s\n", std::format("Sector belongs to \"{}\"", string::join(path.value(), "/")).c_str());
					}
					// TODO: Write and check for error.
				}
			}
		}
	}

	auto iso(
		const std::vector<std::string>& arguments,
		const Detail& detail
	) -> void {
		try {
			auto options = internal::parse_options(arguments);
			auto handle = detail.get_handle(options.drive);
			auto drive = drive::create_drive(handle, detail.ioctl);
			auto drive_info = drive.read_drive_info();
			drive_info.print();
			auto disc_info = drive.read_disc_info();
			if (options.tracks) {
				disc_info = disc::truncate_disc(disc_info, options.tracks.value());
			}
			disc_info.print();
			internal::check_disc(disc_info);
			auto read_offset_correction = options.read_offset_correction ? options.read_offset_correction.value() : drive_info.read_offset_correction ? drive_info.read_offset_correction.value() : 0;
			fprintf(stderr, "%s\n", std::format("Using read offset correction [samples]: {}", read_offset_correction).c_str());
			auto path = std::filesystem::weakly_canonical(std::filesystem::current_path() / options.path).string();
			fprintf(stderr, "%s\n", std::format("Using path: \"{}\"", path).c_str());
			// TODO: Split path into directory, filename and extensions and set default.
			// TODO: Open file.
			auto error_recovery_mode_page = drive.read_error_recovery_mode_page();
			error_recovery_mode_page.page.read_retry_count = 8;
			drive.write_error_recovery_mode_page(error_recovery_mode_page);
			for (auto session_index = size_t(0); session_index < disc_info.sessions.size(); session_index += 1) {
				auto& session = disc_info.sessions.at(session_index);
				for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
					auto& track = session.tracks.at(track_index);
					if (disc::is_data_track(track.type)) {
						internal::copy_data_track(drive, track);
					}
				}
			}
		} catch (const exceptions::ArgumentException& e) {
			fprintf(stderr, "%s\n", "Arguments:");
			throw;
		}
	};
}
