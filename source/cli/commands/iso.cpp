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
		std::optional<std::string> path;
		std::optional<size_t> data_max_pass_count;
		std::optional<size_t> data_max_retry_count;
		std::optional<size_t> data_min_identical_copy_count;
		std::optional<size_t> audio_max_pass_count;
		std::optional<size_t> audio_max_retry_count;
		std::optional<size_t> audio_min_identical_copy_count;

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
			auto path = std::optional<std::string>();
			auto data_max_pass_count = std::optional<si_t>();
			auto data_max_retry_count = std::optional<si_t>();
			auto data_min_identical_copy_count = std::optional<si_t>();
			auto audio_max_pass_count = std::optional<si_t>();
			auto audio_max_retry_count = std::optional<si_t>();
			auto audio_min_identical_copy_count = std::optional<si_t>();
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
				} else if (argument.find("--data-max-pass-count=") == 0) {
					auto value = argument.substr(sizeof("--data-max-pass-count=") - 1);
					auto format = std::string("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						data_max_pass_count = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("data-max-pass-count", format));
					}
				} else if (argument.find("--data-max-retry-count=") == 0) {
					auto value = argument.substr(sizeof("--data-max-retry-count=") - 1);
					auto format = std::string("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						data_max_retry_count = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("data-max-retry-count", format));
					}
				} else if (argument.find("--data-min-identical-copy-count=") == 0) {
					auto value = argument.substr(sizeof("--data-min-identical-copy-count=") - 1);
					auto format = std::string("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						data_min_identical_copy_count = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("data-min-identical-copy-count", format));
					}
				} else if (argument.find("--audio-max-pass-count=") == 0) {
					auto value = argument.substr(sizeof("--audio-max-pass-count=") - 1);
					auto format = std::string("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						audio_max_pass_count = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("audio-max-pass-count", format));
					}
				} else if (argument.find("--audio-max-retry-count=") == 0) {
					auto value = argument.substr(sizeof("--audio-max-retry-count=") - 1);
					auto format = std::string("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						audio_max_retry_count = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("audio-max-retry-count", format));
					}
				} else if (argument.find("--audio-min-identical-copy-count=") == 0) {
					auto value = argument.substr(sizeof("--audio-min-identical-copy-count=") - 1);
					auto format = std::string("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						audio_min_identical_copy_count = std::atoi(matches[0].c_str());
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException("audio-min-identical-copy-count", format));
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
				path,
				data_max_pass_count,
				data_max_retry_count,
				data_min_identical_copy_count,
				audio_max_pass_count,
				audio_max_retry_count,
				audio_min_identical_copy_count
			};
		}

		auto get_absolute_path_with_extension(
			const std::optional<std::string>& path,
			const std::string& extension = ""
		) -> std::string {
			auto fspath = std::filesystem::path(path.value_or(""));
			if (!fspath.has_stem() || fspath.stem().string().starts_with(".")) {
				fspath.replace_filename("image");
			}
			fspath.replace_extension(extension);
			return std::filesystem::weakly_canonical(std::filesystem::current_path() / fspath).string();
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
							auto& bad_sector_indices = bad_sector_indices_per_path[path];
							bad_sector_indices.push_back(bad_sector_index);
						} else {
							auto& bad_sector_indices = bad_sector_indices_per_path[""];
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
			size_t first_sector,
			size_t last_sector,
			size_t max_pass_count,
			size_t max_read_retry_count,
			size_t min_identical_copy_count
		) -> std::vector<std::vector<ExtractedSector>> {
			auto length_sectors = last_sector - first_sector;
			auto extracted_sectors_vector = std::vector<std::vector<ExtractedSector>>(length_sectors);
			drive.set_read_retry_count(max_read_retry_count);
			for (auto pass_index = size_t(0); pass_index < max_pass_count; pass_index += 1) {
				fprintf(stderr, "%s\n", std::format("Running pass {}", pass_index).c_str());
				for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
					try {
						auto sector = ExtractedSector();
						drive.read_sector(sector_index, &sector.sector_data, &sector.subchannels_data, &sector.c2_data);
						auto& extracted_sectors = extracted_sectors_vector.at(sector_index - first_sector);
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
				if (number_of_identical_copies >= min_identical_copy_count) {
					break;
				}
			}
			return extracted_sectors_vector;
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
			auto iso_path = internal::get_absolute_path_with_extension(options.path, ".iso");
			fprintf(stderr, "%s\n", std::format("Using path: \"{}\"", iso_path).c_str());
			auto disc_tracks = disc::get_disc_tracks(disc_info, options.track_numbers);
			if (disc_tracks.size() != 1) {
				OVERDRIVE_THROW(exceptions::InvalidValueException("track count", disc_tracks.size(), 1, 1));
			}
			for (auto track_index = size_t(0); track_index < disc_tracks.size(); track_index += 1) {
				auto& track = disc_tracks.at(track_index);
				fprintf(stderr, "%s\n", std::format("Extracting track number {} containing {} sectors from {} to {}", track.number, track.length_sectors, track.first_sector_relative, track.last_sector_relative).c_str());
				if (disc::is_data_track(track.type)) {
					auto user_data_size = disc::get_user_data_length(track.type);
					if (user_data_size != iso9660::USER_DATA_SIZE) {
						OVERDRIVE_THROW(exceptions::InvalidValueException("user data size", user_data_size, iso9660::USER_DATA_SIZE, iso9660::USER_DATA_SIZE));
					}
					auto extracted_sectors_vector = internal::copy_track(
						drive,
						track.first_sector_relative,
						track.last_sector_relative,
						options.data_max_pass_count.value_or(4),
						options.data_max_retry_count.value_or(16),
						options.data_min_identical_copy_count.value_or(1)
					);
					auto bad_sector_indices = internal::get_bad_sector_indices(extracted_sectors_vector);
					auto bad_sector_indices_per_path = internal::get_bad_sector_indices_per_path(drive, track, bad_sector_indices);
					if (bad_sector_indices_per_path) {
						for (auto entry : bad_sector_indices_per_path.value()) {
							fprintf(stderr, "%s\n", std::format("File at path \"{}\" contains {} bad sectors!", std::filesystem::path(entry.first).string(), entry.second.size()).c_str());
						}
					} else {
						fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
					}
				} else {
					auto read_offset_correction_bytes = read_offset_correction * si_t(cdda::STEREO_SAMPLE_LENGTH);
					auto start_offset_bytes = si_t(track.first_sector_relative * cd::SECTOR_LENGTH) + read_offset_correction_bytes;
					auto end_offset_bytes = si_t(track.last_sector_relative * cd::SECTOR_LENGTH) + read_offset_correction_bytes;
					auto first_sector = idiv::floor(start_offset_bytes, cd::SECTOR_LENGTH);
					auto last_sector = idiv::ceil(end_offset_bytes, cd::SECTOR_LENGTH);
					// TODO: Adjust first_sector and last_sector so that they never overlap with data tracks.
					auto extracted_sectors_vector = internal::copy_track(
						drive,
						first_sector,
						last_sector,
						options.audio_max_pass_count.value_or(8),
						options.audio_max_retry_count.value_or(255),
						options.audio_min_identical_copy_count.value_or(2)
					);
					auto bad_sector_indices = internal::get_bad_sector_indices(extracted_sectors_vector);
					fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
					if (read_offset_correction_bytes != 0) {
						// TODO: Adjust data read.
					}
					// OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
				}
			}
		} catch (const exceptions::ArgumentException& e) {
			fprintf(stderr, "%s\n", "Arguments:");
			throw;
		}
	};
}
