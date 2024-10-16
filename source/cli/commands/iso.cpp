#include "iso.h"

#include <filesystem>
#include <format>
#include <optional>
#include <regex>
#include <set>
#include <string>

namespace commands {
	class ISOOptions {
		public:

		std::string drive;
		std::optional<si_t> read_correction;
		std::optional<std::set<size_t>> track_numbers;
		std::optional<std::string> path;
		size_t min_data_passes;
		size_t max_data_passes;
		size_t max_data_retries;
		size_t min_data_copies;
		size_t max_data_copies;
		size_t min_audio_passes;
		size_t max_audio_passes;
		size_t max_audio_retries;
		size_t min_audio_copies;
		size_t max_audio_copies;

		protected:
	};

	namespace internal {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto options = ISOOptions();
			auto parsers = std::vector<arguments::Parser>();
			parsers.push_back(arguments::Parser({
				"drive",
				"Specify which drive to read from.",
				std::regex("^([A-Z])[:]?$"),
				"string",
				true,
				std::optional<std::string>(),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.drive = matches.at(0);
				}
			}));
			parsers.push_back(arguments::Parser({
				"path",
				"Specify which path to write to.",
				std::regex("^(.+)$"),
				"string",
				true,
				std::optional<std::string>(),
				0,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.path = matches.at(0);
				}
			}));
			parsers.push_back(arguments::Parser({
				"read-correction",
				"Specify read offset correction in number of samples.",
				std::regex("^([+-]?(?:[0-9]|[1-9][0-9]+))$"),
				"integer",
				false,
				std::optional<std::string>(),
				0,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.read_correction = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"track-numbers",
				"Specify which track numbers to read.",
				std::regex("^([1-9]|[1-9][0-9])$"),
				"integer",
				false,
				std::optional<std::string>(),
				0,
				99,
				[&](const std::vector<std::string>& matches) -> void {
					auto track_numbers = std::set<size_t>();
					for (auto& match : matches) {
						track_numbers.insert(std::atoi(match.c_str()));
					}
					options.track_numbers = track_numbers;
				}
			}));
			parsers.push_back(arguments::Parser({
				"min-data-passes",
				"Specify the minimum number of read passes for data tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.min_data_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"max-data-passes",
				"Specify the maximum number of read passes for data tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("4"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.max_data_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"max-data-retries",
				"Specify the maximum number of read retires for data tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("16"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.max_data_retries = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"min-data-copies",
				"Specify the minimum acceptable number of identical copies for data tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.min_data_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"max-data-copies",
				"Specify the maximum acceptable number of identical copies for data tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.max_data_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"min-audio-passes",
				"Specify the minimum number of read passes for audio tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("2"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.min_audio_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"max-audio-passes",
				"Specify the maximum number of read passes for audio tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("8"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.max_audio_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"max-audio-retries",
				"Specify the maximum number of read retires for audio tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("255"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.max_audio_retries = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"min-audio-copies",
				"Specify the minimum acceptable number of identical copies for audio tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.min_audio_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"max-audio-copies",
				"Specify the maximum acceptable number of identical copies for audio tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("2"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.max_audio_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			arguments::sort(parsers);
			try {
				arguments::parse(arguments, parsers);
				return options;
			} catch (const exceptions::ArgumentException& e) {
				arguments::print(parsers);
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

		auto get_absolute_path_with_extension(
			const std::string& path,
			const std::string& extension = ""
		) -> std::string {
			auto fspath = std::filesystem::path(path);
			if (!fspath.has_stem() || fspath.stem().string().starts_with(".")) {
				fspath.replace_filename("image");
			}
			fspath.replace_extension(extension);
			return std::filesystem::weakly_canonical(std::filesystem::current_path() / fspath).string();
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
		fprintf(stderr, "%s\n", std::format("Using read correction for audio tracks [samples]: {}", read_correction_samples).c_str());
		auto read_correction_bytes = read_correction_samples * si_t(cdda::STEREO_SAMPLE_LENGTH);
		fprintf(stderr, "%s\n", std::format("Using read correction for audio tracks [bytes]: {}", read_correction_bytes).c_str());
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
				auto user_data_offset = disc::get_user_data_offset(track.type);
				auto user_data_length = disc::get_user_data_length(track.type);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				auto bad_sector_indices_per_path = copier::get_bad_sector_indices_per_path(drive, user_data_offset, user_data_length, bad_sector_indices);
				if (bad_sector_indices_per_path) {
					for (auto& entry : bad_sector_indices_per_path.value()) {
						fprintf(stderr, "%s\n", std::format("File at path \"{}\" contains {} bad sectors!", std::filesystem::path(entry.first).string(), entry.second.size()).c_str());
					}
				} else {
					fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
				}
				auto iso_path = internal::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.iso", track_index));
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
					read_correction_bytes
				);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
				auto bin_path = internal::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.bin", track_index));
				copier::write_sector_data_to_file(extracted_sectors_vector, bin_path, 0, cd::SECTOR_LENGTH);
			}
		}
	};
}
