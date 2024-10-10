#include "iso.h"

#include <array>
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
		std::optional<size_t> data_min_passes;
		std::optional<size_t> data_max_passes;
		std::optional<size_t> data_max_retries;
		std::optional<size_t> data_max_copies;
		std::optional<size_t> audio_min_passes;
		std::optional<size_t> audio_max_passes;
		std::optional<size_t> audio_max_retries;
		std::optional<size_t> audio_max_copies;

		protected:
	};

	class Parser {
		public:

		std::function<bool_t(const std::string& key, const std::string& value)> parse_named;
		std::function<bool_t(size_t& positional_counter, size_t& positional_index, const std::string& argument)> parse_positional;

		protected:
	};

	template <typename A>
	class Argument {
		public:

		std::string key;
		std::string format;
		bool_t positional;
		std::optional<A> value;
		std::function<A(const std::vector<std::string>& matches)> parser;

		auto make_parser(
		) -> Parser {
			return {
				[&](const std::string& key, const std::string& value) -> bool_t {
					return this->parse_named(key, value);
				},
				[&](size_t& positional_counter, size_t& positional_index, const std::string& argument) -> bool_t {
					return this->parse_positional(positional_counter, positional_index, argument);
				}
			};
		}

		protected:

		auto parse_named(
			const std::string& key,
			const std::string& value
		) -> bool_t {
			if (this->key == key) {
				auto matches = std::vector<std::string>();
				if (string::match(value, matches, std::regex(this->format))) {
					this->value = this->parser(matches);
				} else {
					OVERDRIVE_THROW(exceptions::BadArgumentException(this->key, this->format));
				}
				return true;
			}
			return false;
		}

		auto parse_positional(
			size_t& positional_counter,
			size_t& positional_index,
			const std::string& argument
		) -> bool_t {
			if (this->positional) {
				if (positional_counter == positional_index) {
					auto matches = std::vector<std::string>();
					if (string::match(argument, matches, std::regex(this->format))) {
						this->value = this->parser(matches);
					} else {
						OVERDRIVE_THROW(exceptions::BadArgumentException(this->key, this->format));
					}
					positional_index += 1;
					return true;
				}
				positional_counter += 1;
			}
			return false;
		}
	};

	namespace internal {
		auto parse_options_using_parsers(
			const std::vector<std::string>& arguments,
			const std::vector<Parser>& parsers
		) -> void {
			auto positional_index = size_t(0);
			for (auto argument_index = size_t(0); argument_index < arguments.size(); argument_index += 1) {
				auto& argument = arguments[argument_index];
				auto parsed = false;
				auto matches = std::vector<std::string>();
				if (string::match(argument, matches, std::regex("^[-][-]([^=]+)[=]([^=]+)$"))) {
					auto& key = matches.at(0);
					auto& value = matches.at(1);
					for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
						auto& parser = parsers.at(parser_index);
						parsed = parser.parse_named(key, value);
						if (parsed) {
							break;
						}
					}
				} else {
					auto positional_counter = size_t(0);
					for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
						auto& parser = parsers.at(parser_index);
						parsed = parser.parse_positional(positional_counter, positional_index, argument);
						if (parsed) {
							break;
						}
					}
				}
				if (!parsed) {
					OVERDRIVE_THROW(exceptions::UnknownArgumentException(argument));
				}
			}
		}

		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto parsers = std::vector<Parser>();
			auto drive = Argument<std::string>({
				"drive",
				"^([A-Z])[:]?$",
				true,
				std::optional<std::string>(),
				[&](const std::vector<std::string>& matches) -> std::string {
					return matches.at(0);
				}
			});
			parsers.push_back(drive.make_parser());
			auto path = Argument<std::string>({
				"path",
				"^(.+)$",
				true,
				std::optional<std::string>(),
				[&](const std::vector<std::string>& matches) -> std::string {
					return matches.at(0);
				}
			});
			parsers.push_back(path.make_parser());
			auto read_correction = Argument<si_t>({
				"read-correction",
				"^([+-]?(?:[0-9]|[1-9][0-9]+))$",
				false,
				std::optional<si_t>(),
				[&](const std::vector<std::string>& matches) -> si_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(read_correction.make_parser());
			auto track_numbers = Argument<std::set<size_t>>({
				"track-numbers",
				"^([1-9]|[1-9][0-9])(?:[,]([1-9]|[1-9][0-9]))*$",
				false,
				std::optional<std::set<size_t>>(),
				[&](const std::vector<std::string>& matches) -> std::set<size_t> {
					auto track_numbers = std::set<size_t>();
					for (auto& match : matches) {
						track_numbers.insert(std::atoi(match.c_str()));
					}
					return track_numbers;
				}
			});
			parsers.push_back(track_numbers.make_parser());
			auto data_min_passes = Argument<size_t>({
				"data-min-passes",
				"^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(data_min_passes.make_parser());
			auto data_max_passes = Argument<size_t>({
				"data-max-passes",
				"^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(data_max_passes.make_parser());
			auto data_max_retries = Argument<size_t>({
				"data-max-retries",
				"^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(data_max_retries.make_parser());
			auto data_max_copies = Argument<size_t>({
				"data-max-copies",
				"^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(data_max_copies.make_parser());
			auto audio_min_passes = Argument<size_t>({
				"audio-min-passes",
				"^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(audio_min_passes.make_parser());
			auto audio_max_passes = Argument<size_t>({
				"audio-max-passes",
				"^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(audio_max_passes.make_parser());
			auto audio_max_retries = Argument<size_t>({
				"audio-max-retries",
				"^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(audio_max_retries.make_parser());
			auto audio_max_copies = Argument<size_t>({
				"audio-max-copies",
				"^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$",
				false,
				std::optional<size_t>(),
				[&](const std::vector<std::string>& matches) -> size_t {
					return std::atoi(matches.at(0).c_str());
				}
			});
			parsers.push_back(audio_max_copies.make_parser());
			try {
				parse_options_using_parsers(arguments, parsers);
				if (!drive.value) {
					OVERDRIVE_THROW(exceptions::MissingArgumentException("drive"));
				}
				return {
					drive.value.value(),
					read_correction.value,
					track_numbers.value,
					path.value,
					data_min_passes.value,
					data_max_passes.value,
					data_max_retries.value,
					data_max_copies.value,
					audio_min_passes.value,
					audio_max_passes.value,
					audio_max_retries.value,
					audio_max_copies.value
				};
			} catch (const exceptions::ArgumentException& e) {
				fprintf(stderr, "%s\n", "Arguments:");
				throw;
			}
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
		auto read_correction = options.read_correction ? options.read_correction.value() : drive_info.read_offset_correction ? drive_info.read_offset_correction.value() : 0;
		fprintf(stderr, "%s\n", std::format("Using read correction [samples]: {}", read_correction).c_str());
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
				auto user_data_offset = disc::get_user_data_offset(track.type);
				auto user_data_length = disc::get_user_data_length(track.type);
				if (user_data_length != iso9660::USER_DATA_SIZE) {
					OVERDRIVE_THROW(exceptions::InvalidValueException("user data length", user_data_length, iso9660::USER_DATA_SIZE, iso9660::USER_DATA_SIZE));
				}
				auto extracted_sectors_vector = copier::read_sector_range(
					drive,
					track.first_sector_relative,
					track.last_sector_relative,
					options.data_min_passes.value_or(1),
					options.data_max_passes.value_or(4),
					options.data_max_retries.value_or(16),
					options.data_max_copies.value_or(1)
				);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector);
				auto bad_sector_indices_per_path = copier::get_bad_sector_indices_per_path(drive, user_data_offset, user_data_length, bad_sector_indices);
				if (bad_sector_indices_per_path) {
					for (auto entry : bad_sector_indices_per_path.value()) {
						fprintf(stderr, "%s\n", std::format("File at path \"{}\" contains {} bad sectors!", std::filesystem::path(entry.first).string(), entry.second.size()).c_str());
					}
				} else {
					fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
				}
				auto iso_handle = std::fopen(iso_path.c_str(), "wb+");
				if (iso_handle == nullptr) {
					OVERDRIVE_THROW(exceptions::OverdriveException(""));
				}
				try {
					auto empty_sector = std::array<byte_t, iso9660::USER_DATA_SIZE>();
					for (auto sector_index = size_t(0); sector_index < extracted_sectors_vector.size(); sector_index += 1) {
						auto& extracted_sectors = extracted_sectors_vector.at(sector_index);
						if (extracted_sectors.size() > 0) {
							auto& extracted_sector = extracted_sectors.at(0);
							if (std::fwrite(extracted_sector.sector_data + user_data_offset, user_data_length, 1, iso_handle) != 1) {
								OVERDRIVE_THROW(exceptions::OverdriveException(""));
							}
						} else {
							if (std::fwrite(empty_sector.data(), sizeof(empty_sector), 1, iso_handle) != 1) {
								OVERDRIVE_THROW(exceptions::OverdriveException(""));
							}
						}
					}
				} catch (...) {
					std::fclose(iso_handle);
					throw;
				}
			} else {
				auto read_correction_bytes = read_correction * si_t(cdda::STEREO_SAMPLE_LENGTH);
				auto start_offset_bytes = si_t(track.first_sector_relative * cd::SECTOR_LENGTH) + read_correction_bytes;
				auto end_offset_bytes = si_t(track.last_sector_relative * cd::SECTOR_LENGTH) + read_correction_bytes;
				auto first_sector = idiv::floor(start_offset_bytes, cd::SECTOR_LENGTH);
				auto last_sector = idiv::ceil(end_offset_bytes, cd::SECTOR_LENGTH);
				// TODO: Adjust first_sector and last_sector so that they never overlap with data tracks.
				auto extracted_sectors_vector = copier::read_sector_range(
					drive,
					first_sector,
					last_sector,
					options.audio_min_passes.value_or(2),
					options.audio_max_passes.value_or(8),
					options.audio_max_retries.value_or(255),
					options.audio_max_copies.value_or(2)
				);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector);
				fprintf(stderr, "%s\n", std::format("Track {} contains {} bad sectors!", track.number, bad_sector_indices.size()).c_str());
				if (read_correction_bytes != 0) {
					// TODO: Adjust data read.
				}
				// OVERDRIVE_THROW(exceptions::ExpectedDataTrackException(track.number));
			}
		}
	};
}
