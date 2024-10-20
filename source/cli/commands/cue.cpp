#include "cue.h"

#include <cstdio>
#include <format>
#include <optional>
#include <regex>

namespace commands {
	class CUEOptions: public options::Options {
		public:

		std::optional<std::set<size_t>> track_numbers;
		bool_t merge_tracks;
		bool_t trim_data_tracks;
		std::string audio_file_format;

		protected:
	};

	namespace internal {
	namespace {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> CUEOptions {
			auto options = CUEOptions();
			auto parsers = options::get_default_parsers(options);
			parsers.push_back(parser::Parser({
				"track-numbers",
				{},
				"Specify which track numbers to save.",
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
			parsers.push_back(parser::Parser({
				"merge-tracks",
				{},
				"Specify whether to merge all tracks into a single file.",
				std::regex("^(true|false)$"),
				"boolean",
				false,
				std::optional<std::string>("false"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.merge_tracks = matches.at(0) == "true";
				}
			}));
			parsers.push_back(parser::Parser({
				"trim-data-tracks",
				{},
				"Specify whether to trim sector data other than user data from data tracks.",
				std::regex("^(true|false)$"),
				"boolean",
				false,
				std::optional<std::string>("true"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.trim_data_tracks = matches.at(0) == "true";
				}
			}));
			parsers.push_back(parser::Parser({
				"audio-file-format",
				{},
				"Specify file format for audio tracks.",
				std::regex("^(bin|wav)$"),
				"bin|wav",
				false,
				std::optional<std::string>("wav"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_file_format = matches.at(0);
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
				if (track.type == disc::TrackType::AUDIO_4_CHANNELS) {
					OVERDRIVE_THROW(exceptions::UnsupportedValueException("track type AUDIO_4_CHANNELS"));
				}
				if (track.type == disc::TrackType::DATA_MODE0) {
					OVERDRIVE_THROW(exceptions::UnsupportedValueException("track type DATA_MODE0"));
				}
			}
		}

		auto get_track_tag(
			disc::TrackType type,
			bool_t trim_data_tracks
		) -> std::string {
			if (type == disc::TrackType::AUDIO_2_CHANNELS) {
				return "AUDIO";
			}
			if (type == disc::TrackType::AUDIO_4_CHANNELS) {
				OVERDRIVE_THROW(exceptions::UnsupportedValueException("track type AUDIO_4_CHANNELS"));
			}
			if (type == disc::TrackType::DATA_MODE0) {
				OVERDRIVE_THROW(exceptions::UnsupportedValueException("track type DATA_MODE0"));
			}
			if (type == disc::TrackType::DATA_MODE1) {
				return trim_data_tracks ? "MODE1/2048" : "MODE1/2352";
			}
			if (type == disc::TrackType::DATA_MODE2) {
				return trim_data_tracks ? "MODE2/2336" : "MODE2/2352";
			}
			if (type == disc::TrackType::DATA_MODE2_FORM1) {
				return trim_data_tracks ? "MODE2/2048" : "MODE2/2352";
			}
			if (type == disc::TrackType::DATA_MODE2_FORM2) {
				return trim_data_tracks ? "MODE2/2324" : "MODE2/2352";
			}
			OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
		}

		auto write_merged_bin(
			const drive::Drive& drive,
			const std::vector<disc::TrackInfo>& tracks,
			const CUEOptions& options
		) -> void {
			auto path = path::create_path(options.path)
				.with_extension(".bin")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
					auto& track = tracks.at(track_index);
					auto extracted_sectors_vector = copier::read_track(drive, track, options);
					auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
					copier::log_bad_sector_indices(drive, track, bad_sector_indices);
					if (disc::is_data_track(track.type)) {
						auto sector_data_offset = options.trim_data_tracks ? disc::get_user_data_offset(track.type) : 0;
						auto sector_data_length = options.trim_data_tracks ? disc::get_user_data_length(track.type) : cd::SECTOR_LENGTH;
						copier::append_sector_data(extracted_sectors_vector, path, sector_data_offset, sector_data_length, handle, false);
					} else {
						copier::append_sector_data(extracted_sectors_vector, path, 0, cd::SECTOR_LENGTH, handle, false);
					}
				}
			}  catch (...) {
				copier::close_handle(handle);
				throw;
			}
			copier::close_handle(handle);
		}

		auto write_merged_cue(
			const std::vector<disc::TrackInfo>& tracks,
			const CUEOptions& options
		) -> void {
			auto path = path::create_path(options.path)
				.with_extension(".cue")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				if (std::fprintf(handle, "%s\n", std::format("FILE \"{}\" {}", std::format("{}.bin", path.fspath.stem().string()), "BINARY").c_str()) < 0) {
					OVERDRIVE_THROW(exceptions::IOWriteException(path));
				}
				auto offset = size_t(0);
				for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
					auto& track = tracks.at(track_index);
					auto track_tag = internal::get_track_tag(track.type, options.trim_data_tracks);
					if (std::fprintf(handle, "%s\n", std::format("\tTRACK {:0>2} {}", track_index + 1, track_tag).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
					if (std::fprintf(handle, "%s\n", std::format("\t\tPREGAP {:0>2}:{:0>2}:{:0>2}", 0, 0, 0).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
					auto address = cd::get_address_from_sector(offset);
					if (std::fprintf(handle, "%s\n", std::format("\t\tINDEX {:0>2} {:0>2}:{:0>2}:{:0>2}", 1, address.m, address.s, address.f).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
					offset += track.length_sectors;
				}
			} catch (...) {
				copier::close_handle(handle);
				throw;
			}
			copier::close_handle(handle);
		}

		auto write_bin(
			const drive::Drive& drive,
			const std::vector<disc::TrackInfo>& tracks,
			const CUEOptions& options
		) -> void {
			for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
				auto& track = tracks.at(track_index);
				auto extracted_sectors_vector = copier::read_track(drive, track, options);
				auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
				copier::log_bad_sector_indices(drive, track, bad_sector_indices);
				if (disc::is_data_track(track.type)) {
					auto sector_data_offset = options.trim_data_tracks ? disc::get_user_data_offset(track.type) : 0;
					auto sector_data_length = options.trim_data_tracks ? disc::get_user_data_length(track.type) : cd::SECTOR_LENGTH;
					auto path = path::create_path(options.path)
						.with_extension(std::format(".{:0>2}.bin", track.number))
						.create_directories();
					auto handle = copier::open_handle(path);
					copier::append_sector_data(extracted_sectors_vector, path, sector_data_offset, sector_data_length, handle, false);
					copier::close_handle(handle);
				} else {
					auto extension = options.audio_file_format == "wav" ? "wav" : "bin";
					auto path = path::create_path(options.path)
						.with_extension(std::format(".{:0>2}.{}", track.number, extension))
						.create_directories();
					auto handle = copier::open_handle(path);
					if (options.audio_file_format == "wav") {
						auto header = wav::Header();
						header.data_length = cd::SECTOR_LENGTH * track.length_sectors;
						header.riff_length = header.data_length + sizeof(wav::Header) - offsetof(wav::Header, wave_identifier);
						if (std::fwrite(&header, sizeof(wav::Header), 1, handle) != 1) {
							OVERDRIVE_THROW(exceptions::IOWriteException(path));
						}
					}
					copier::append_sector_data(extracted_sectors_vector, path, 0, cd::SECTOR_LENGTH, handle, false);
					copier::close_handle(handle);
				}
			}
		}

		auto write_cue(
			const std::vector<disc::TrackInfo>& tracks,
			const CUEOptions& options
		) -> void {
			auto path = path::create_path(options.path)
				.with_extension(".cue")
				.create_directories();
			auto handle = copier::open_handle(path);
			try {
				for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
					auto& track = tracks.at(track_index);
					auto file_tag = disc::is_data_track(track.type) ? "BINARY" : options.audio_file_format == "wav" ? "WAVE" : "BINARY";
					auto track_tag = internal::get_track_tag(track.type, options.trim_data_tracks);
					auto extension = options.audio_file_format == "wav" ? "wav" : "bin";
					if (std::fprintf(handle, "%s\n", std::format("FILE \"{}\" {}", std::format("{}.{:0>2}.{}", path.fspath.stem().string(), track.number, extension), file_tag).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
					if (std::fprintf(handle, "%s\n", std::format("\tTRACK {:0>2} {}", track_index + 1, track_tag).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
					if (std::fprintf(handle, "%s\n", std::format("\t\tPREGAP {:0>2}:{:0>2}:{:0>2}", 0, 0, 0).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
					if (std::fprintf(handle, "%s\n", std::format("\t\tINDEX {:0>2} {:0>2}:{:0>2}:{:0>2}", 1, 0, 0, 0).c_str()) < 0) {
						OVERDRIVE_THROW(exceptions::IOWriteException(path));
					}
				}
			} catch (...) {
				copier::close_handle(handle);
				throw;
			}
			copier::close_handle(handle);
		}
	}
	}

	auto cue(
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
		if (options.merge_tracks) {
			internal::write_merged_bin(drive, tracks, options);
			internal::write_merged_cue(tracks, options);
		} else {
			internal::write_bin(drive, tracks, options);
			internal::write_cue(tracks, options);
		}
	};
}
