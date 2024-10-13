#include "iso.h"

#include <cstdio>
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
		size_t data_min_passes;
		size_t data_max_passes;
		size_t data_max_retries;
		size_t data_min_copies;
		size_t data_max_copies;
		size_t audio_min_passes;
		size_t audio_max_passes;
		size_t audio_max_retries;
		size_t audio_min_copies;
		size_t audio_max_copies;

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
				true,
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
				false,
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
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.read_correction = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"track-numbers",
				"Specify which track numbers to read.",
				std::regex("^([1-9]|[1-9][0-9])(?:[,]([1-9]|[1-9][0-9]))*$"),
				"list<integer>",
				false,
				std::optional<std::string>(),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					auto track_numbers = std::set<size_t>();
					for (auto& match : matches) {
						track_numbers.insert(std::atoi(match.c_str()));
					}
					options.track_numbers = track_numbers;
				}
			}));
			parsers.push_back(arguments::Parser({
				"data-min-passes",
				"Specify the minimum number of read passes for data tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.data_min_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"data-max-passes",
				"Specify the maximum number of read passes for data tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("4"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.data_max_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"data-max-retries",
				"Specify the maximum number of read retires for data tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("16"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.data_max_retries = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"data-min-copies",
				"Specify the minimum acceptable number of identical copies for data tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.data_min_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"data-max-copies",
				"Specify the maximum acceptable number of identical copies for data tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.data_max_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"audio-min-passes",
				"Specify the minimum number of read passes for audio tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("2"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_min_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"audio-max-passes",
				"Specify the maximum number of read passes for audio tracks.",
				std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("8"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_max_passes = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"audio-max-retries",
				"Specify the maximum number of read retires for audio tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("255"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_max_retries = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"audio-min-copies",
				"Specify the minimum acceptable number of identical copies for audio tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("1"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_min_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			parsers.push_back(arguments::Parser({
				"audio-max-copies",
				"Specify the maximum acceptable number of identical copies for audio tracks.",
				std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
				"integer",
				false,
				std::optional<std::string>("2"),
				false,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_max_copies = std::atoi(matches.at(0).c_str());
				}
			}));
			// Sort in increasing order.
			std::sort(parsers.begin(), parsers.end(), [](const arguments::Parser& one, const arguments::Parser& two) -> bool_t {
				return one.key < two.key;
			});
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
			if (tracks.size() != 1) {
				OVERDRIVE_THROW(exceptions::InvalidValueException("track count", tracks.size(), 1, 1));
			}
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
					options.data_min_passes,
					options.data_max_passes,
					options.data_max_retries,
					options.data_min_copies,
					options.data_max_copies
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
				fprintf(stderr, "%s\n", std::format("Saving track {} to: \"{}\"", track.number, iso_path).c_str());
				auto iso_handle = std::fopen(iso_path.c_str(), "wb+");
				if (iso_handle == nullptr) {
					OVERDRIVE_THROW(exceptions::IOOpenException(iso_path));
				}
				try {
					for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
						auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
						auto& extracted_sector = extracted_sectors.at(0);
						if (std::fwrite(extracted_sector.sector_data + user_data_offset, user_data_length, 1, iso_handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(iso_path));
						}
					}
				} catch (...) {
					std::fclose(iso_handle);
					throw;
				}
			} else {
				auto extracted_sectors_vector = copier::read_absolute_sector_range_with_correction(
					drive,
					track.first_sector_absolute,
					track.last_sector_absolute,
					options.data_min_passes,
					options.data_max_passes,
					options.data_max_retries,
					options.data_min_copies,
					options.data_max_copies,
					read_correction_bytes
				);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
				auto bin_path = internal::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.bin", track_index));
				fprintf(stderr, "%s\n", std::format("Saving track {} to: \"{}\"", track.number, bin_path).c_str());
				auto bin_handle = std::fopen(bin_path.c_str(), "wb+");
				if (bin_handle == nullptr) {
					OVERDRIVE_THROW(exceptions::IOOpenException(bin_path));
				}
				try {
					for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
						auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
						auto& extracted_sector = extracted_sectors.at(0);
						if (std::fwrite(extracted_sector.sector_data, cd::SECTOR_LENGTH, 1, bin_handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(bin_path));
						}
					}
				} catch (...) {
					std::fclose(bin_handle);
					throw;
				}
			}
		}
	};
}
