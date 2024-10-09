#include "iso.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>

namespace commands {
	class ISOOptions {
		public:

		std::string drive;
		std::optional<si_t> read_offset_correction;
		std::optional<std::set<size_t>> track_numbers;
		std::string path;

		protected:
	};

	class ExtractedSector {
		public:

		byte_t sector_data[cd::SECTOR_LENGTH];
		byte_t subchannels_data[cd::SUBCHANNELS_LENGTH];
		byte_t c2_data[cd::C2_LENGTH];
		size_t counter;

		auto has_identical_sector_data(
			const ExtractedSector& that
		) -> bool_t {
			return std::memcmp(this->sector_data, that.sector_data, sizeof(sector_data)) == 0;
		}

		auto has_identical_subchannels_data(
			const ExtractedSector& that
		) -> bool_t {
			return std::memcmp(this->subchannels_data, that.subchannels_data, sizeof(subchannels_data)) == 0;
		}

		auto has_identical_c2_data(
			const ExtractedSector& that
		) -> bool_t {
			return std::memcmp(this->c2_data, that.c2_data, sizeof(c2_data)) == 0;
		}

		protected:
	};

	namespace internal {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto drive = std::optional<std::string>();
			auto read_offset_correction = std::optional<si_t>();
			auto track_numbers = std::optional<std::set<size_t>>();
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
						drive = matches[0];
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("drive", format));
					}
				} else if (argument.find("--read-offset-correction=") == 0) {
					auto value = argument.substr(sizeof("--read-offset-correction=") - 1);
					auto format = std::string("^([+-]?(?:[0-9]|[1-9][0-9]+))$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						read_offset_correction = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("read-offset-correction", format));
					}
				} else if (argument.find("--track-numbers=") == 0) {
					auto value = argument.substr(sizeof("--track-numbers=") - 1);
					auto format = std::string("^([1-9]|[1-9][0-9])(?:[,]([1-9]|[1-9][0-9]))*$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						track_numbers = std::set<size_t>();
						for (auto& match : matches) {
							track_numbers->insert(std::atoi(match.c_str()));
						}
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("track-numbers", format));
					}
				} else if (argument.find("--path=") == 0) {
					auto value = argument.substr(sizeof("--path=") - 1);
					auto format = std::string("^(.+)$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						path = matches[0];
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
				track_numbers,
				path
			};
		}

		auto check_disc(
			const disc::DiscInfo& disc_info,
			const std::optional<std::set<size_t>>& track_numbers
		) -> void {
			for (auto session_index = size_t(0); session_index < disc_info.sessions.size(); session_index += 1) {
				auto& session = disc_info.sessions.at(session_index);
				for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
					auto& track = session.tracks.at(track_index);
					if (track_numbers && !track_numbers->contains(track.number)) {
						continue;
					}
					if (disc::is_data_track(track.type)) {
						auto user_data_size = disc::get_user_data_length(track.type);
						if (user_data_size != iso9660::USER_DATA_SIZE) {
							OVERDRIVE_THROW(exceptions::InvalidValueException("user data size", user_data_size, iso9660::USER_DATA_SIZE, iso9660::USER_DATA_SIZE));
						}
					} else {
						OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
					}
					return;
				}
			}
			OVERDRIVE_THROW(exceptions::MissingValueException("data track"));
		}

		auto set_read_retry_count(
			const drive::Drive& drive,
			size_t max_retry_count
		) -> void {
			auto error_recovery_mode_page = drive.read_error_recovery_mode_page();
			error_recovery_mode_page.page.read_retry_count = max_retry_count;
			drive.write_error_recovery_mode_page(error_recovery_mode_page);
		}

		auto get_number_of_identical_copies(
			std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector
		) -> size_t {
			auto counters = std::vector<size_t>(1);
			for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
				auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
				if (extracted_sectors.size() > 0) {
					// Sort in decreasing order.
					std::sort(extracted_sectors.begin(), extracted_sectors.end(), [](const ExtractedSector& one, const ExtractedSector& two) -> bool_t {
						return two.counter < one.counter;
					});
					auto& extracted_sector = extracted_sectors.at(0);
					counters.resize(std::max(extracted_sector.counter + 1, counters.size()));
					counters.at(extracted_sector.counter) += 1;
				} else {
					counters.at(0) += 1;
				}
			}
			for (auto counter_index = size_t(0); counter_index < counters.size(); counter_index += 1) {
				auto counter = counters.at(counter_index);
				if (counter != 0) {
					return counter_index;
				}
			}
			return 0;
		}

		auto get_bad_sector_indices(
			const std::vector<std::vector<ExtractedSector>>& extracted_sectors_vector
		) -> std::vector<size_t> {
			auto bad_sector_indices = std::vector<size_t>();
			for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
				auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
				if (extracted_sectors.size() == 0) {
					bad_sector_indices.push_back(sector_index);
				}
			}
			return bad_sector_indices;
		}

		auto get_bad_sector_indices_per_path(
			const drive::Drive& drive,
			const disc::TrackInfo& track_info,
			const std::vector<size_t>& bad_sector_indices
		) -> std::optional<std::map<std::string, std::vector<size_t>>> {
			try {
				auto sector = ExtractedSector();
				auto user_data_offset = disc::get_user_data_offset(track_info.type);
				auto user_data_length = disc::get_user_data_length(track_info.type);
				if (user_data_length == iso9660::USER_DATA_SIZE) {
					auto fs = iso9660::FileSystem([&](size_t sector_index, void* user_data) -> void {
						drive.read_sector(sector_index, &sector.sector_data, nullptr, nullptr);
						std::memcpy(user_data, sector.sector_data + user_data_offset, iso9660::USER_DATA_SIZE);
					});
					auto bad_sector_indices_per_path = std::map<std::string, std::vector<size_t>>();
					for (auto bad_sector_index : bad_sector_indices) {
						auto optional_path = fs.get_path(bad_sector_index);
						if (optional_path) {
							auto path = std::string("/") + string::join(optional_path.value(), "/");
							auto bad_sector_indices = bad_sector_indices_per_path[path];
							bad_sector_indices.push_back(bad_sector_index);
						} else {
							auto bad_sector_indices = bad_sector_indices_per_path[""];
							bad_sector_indices.push_back(bad_sector_index);
						}
					}
					return bad_sector_indices_per_path;
				}
			} catch (const exceptions::SCSIException& e) {
				fprintf(stderr, "%s\n", std::format("Error reading ISO 9660 file system!").c_str());
			}
			return std::optional<std::map<std::string, std::vector<size_t>>>();
		}

		auto copy_track(
			const drive::Drive& drive,
			const disc::TrackInfo& track_info
		) -> void {
			auto max_retry_count = size_t(8);
			auto max_pass_count = size_t(1);
			auto acceptable_copy_count = size_t(1);
			auto read_offset_correction_bytes = si_t(0);
			fprintf(stderr, "%s\n", std::format("Extracting track number {} containing {} sectors from {} to {}", track_info.number, track_info.length_sectors, track_info.first_sector_relative, track_info.last_sector_relative).c_str());
			set_read_retry_count(drive, max_retry_count);
			auto start_offset_bytes = si_t(track_info.first_sector_relative * cd::SECTOR_LENGTH) + read_offset_correction_bytes;
			auto end_offset_bytes = si_t(track_info.last_sector_relative * cd::SECTOR_LENGTH) + read_offset_correction_bytes;
			auto adjusted_first_sector = idiv::floor(start_offset_bytes, cd::SECTOR_LENGTH);
			auto adjusted_last_sector = idiv::ceil(end_offset_bytes, cd::SECTOR_LENGTH);
			auto adjusted_length_sectors = adjusted_last_sector - adjusted_first_sector;
			auto extracted_sectors_vector = std::vector<std::vector<ExtractedSector>>(adjusted_length_sectors);
			for (auto pass_index = size_t(0); pass_index < max_pass_count; pass_index += 1) {
				fprintf(stderr, "%s\n", std::format("Running pass {}", pass_index).c_str());
				for (auto sector_index = adjusted_first_sector; sector_index < adjusted_last_sector; sector_index += 1) {
					try {
						auto sector = ExtractedSector();
						drive.read_sector(sector_index, &sector.sector_data, &sector.subchannels_data, &sector.c2_data);
						auto& extracted_sectors = extracted_sectors_vector.at(sector_index - adjusted_first_sector);
						auto found = false;
						for (auto& extracted_sector : extracted_sectors) {
							if (extracted_sector.has_identical_sector_data(sector)) {
								found = true;
								extracted_sector.counter += 1;
								break;
							}
						}
						if (!found) {
							sector.counter = 1;
							extracted_sectors.push_back(std::move(sector));
						}
					} catch (const exceptions::SCSIException& e) {
						fprintf(stderr, "%s\n", std::format("Error reading sector {}!", sector_index).c_str());
					}
				}
				auto number_of_identical_copies = get_number_of_identical_copies(extracted_sectors_vector);
				fprintf(stderr, "%s\n", std::format("Got {} identical copies", number_of_identical_copies).c_str());
				if (number_of_identical_copies >= acceptable_copy_count) {
					break;
				}
			}
			auto bad_sector_indices = get_bad_sector_indices(extracted_sectors_vector);
			if (disc::is_data_track(track_info.type)) {
				auto bad_sector_indices_per_path = get_bad_sector_indices_per_path(drive, track_info, bad_sector_indices);
				if (bad_sector_indices_per_path) {
					for (auto entry : bad_sector_indices_per_path.value()) {
						fprintf(stderr, "%s\n", std::format("Path \"{}\" has bad sectors!", entry.first).c_str());
					}
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
			disc_info.print();
			auto read_offset_correction = options.read_offset_correction ? options.read_offset_correction.value() : drive_info.read_offset_correction ? drive_info.read_offset_correction.value() : 0;
			fprintf(stderr, "%s\n", std::format("Using read offset correction [samples]: {}", read_offset_correction).c_str());
			auto path = std::filesystem::weakly_canonical(std::filesystem::current_path() / options.path).string();
			fprintf(stderr, "%s\n", std::format("Using path: \"{}\"", path).c_str());
			// TODO: Split path into directory, filename and extensions and set default.
			// TODO: Open file.
			auto disc_tracks = disc::get_disc_tracks(disc_info, options.track_numbers);
			if (disc_tracks.size() == 0) {
				throw exceptions::MissingValueException("disc tracks");
			}
			for (auto track_index = size_t(0); track_index < disc_tracks.size(); track_index += 1) {
				auto& track = disc_tracks.at(track_index);
				if (disc::is_data_track(track.type)) {
					auto user_data_size = disc::get_user_data_length(track.type);
					if (user_data_size != iso9660::USER_DATA_SIZE) {
						OVERDRIVE_THROW(exceptions::InvalidValueException("user data size", user_data_size, iso9660::USER_DATA_SIZE, iso9660::USER_DATA_SIZE));
					}
					internal::copy_track(drive, track);
				} else {
					OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
				}
			}
		} catch (const exceptions::ArgumentException& e) {
			fprintf(stderr, "%s\n", "Arguments:");
			throw;
		}
	};
}
