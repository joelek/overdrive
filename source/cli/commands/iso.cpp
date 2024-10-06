#include "iso.h"

#include <format>
#include <optional>
#include <regex>

namespace commands {
	class ISOOptions {
		public:

		std::string drive;
		std::optional<si_t> read_offset_correction;
		std::optional<ui_t> tracks;

		protected:
	};

	namespace internal {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto drive = std::optional<std::string>();
			auto read_offset_correction = std::optional<si_t>();
			auto tracks = std::optional<ui_t>();
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
				tracks
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
						if (user_data_size != 2048) {
							OVERDRIVE_THROW(exceptions::InvalidValueException("user data size", user_data_size, 2048, 2048));
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
		const std::function<void*(const std::string& drive)>& get_handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) -> void {
		try {
			auto options = internal::parse_options(arguments);
			auto handle = get_handle(options.drive);
			auto drive = drive::create_drive(handle, ioctl);
			auto drive_info = drive.read_drive_info();
			fprintf(stderr, "%s\n", std::format("Drive vendor: \"{}\"", string::trim(drive_info.vendor)).c_str());
			fprintf(stderr, "%s\n", std::format("Drive product: \"{}\"", string::trim(drive_info.product)).c_str());
			fprintf(stderr, "%s\n", std::format("Drive sector data offset: {}", drive_info.sector_data_offset).c_str());
			fprintf(stderr, "%s\n", std::format("Drive subchannels data offset: {}", drive_info.subchannels_data_offset).c_str());
			fprintf(stderr, "%s\n", std::format("Drive c2 data offset: {}", drive_info.c2_data_offset).c_str());
			fprintf(stderr, "%s\n", std::format("Drive buffer size [bytes]: {}", drive_info.buffer_size).c_str());
			fprintf(stderr, "%s\n", std::format("Drive supports accurate stream: {}", drive_info.supports_accurate_stream).c_str());
			fprintf(stderr, "%s\n", std::format("Drive supports c2 error reporting: {}", drive_info.supports_c2_error_reporting).c_str());
			fprintf(stderr, "%s\n", std::format("Drive read offset correction [samples]: {}", drive_info.read_offset_correction ? std::format("{}", drive_info.read_offset_correction.value()) : "unknown").c_str());
			auto disc_info = drive.read_disc_info();
			fprintf(stderr, "%s\n", std::format("Disc sessions: {}", disc_info.sessions.size()).c_str());
			fprintf(stderr, "%s\n", std::format("Disc length [sectors]: {}", disc_info.length_sectors).c_str());
			for (auto session_index = size_t(0); session_index < disc_info.sessions.size(); session_index += 1) {
				auto& session = disc_info.sessions.at(session_index);
				fprintf(stderr, "%s\n", std::format("\tSession number: {}", session.number).c_str());
				fprintf(stderr, "%s\n", std::format("\tSession type: {}", enums::SessionType(session.type)).c_str());
				fprintf(stderr, "%s\n", std::format("\tSession tracks: {}", session.tracks.size()).c_str());
				fprintf(stderr, "%s\n", std::format("\tSession length [sectors]: {}", session.length_sectors).c_str());
				for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
					auto& track = session.tracks.at(track_index);
					fprintf(stderr, "%s\n", std::format("\t\tTrack number: {}", track.number).c_str());
					fprintf(stderr, "%s\n", std::format("\t\tTrack type: {}", enums::TrackType(track.type)).c_str());
					fprintf(stderr, "%s\n", std::format("\t\tTrack first sector (absolute): {}", track.first_sector_absolute).c_str());
					fprintf(stderr, "%s\n", std::format("\t\tTrack length [sectors]: {}", track.length_sectors).c_str());
				}
			}
			if (options.tracks) {
				disc_info = disc::truncate_disc(disc_info, options.tracks.value());
			}
			internal::check_disc(disc_info);
		} catch (const exceptions::ArgumentException& e) {
			fprintf(stderr, "%s\n", "Arguments:");
			throw;
		}
	};
}
