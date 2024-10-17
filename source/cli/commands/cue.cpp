#include "cue.h"

#include <cstdio>
#include <format>

namespace commands {
	class CUEOptions: public options::Options {
		public:

		bool_t merge_tracks;
		bool_t store_raw_tracks;
		std::string audio_format;

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
				"merge-tracks",
				"Specify whether to merge all tracks into a single file.",
				std::regex("^(true|false)$"),
				"boolean",
				false,
				std::optional<std::string>("true"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.merge_tracks = matches.at(0) == "true";
				}
			}));
			parsers.push_back(parser::Parser({
				"store-raw-tracks",
				"Specify whether to store raw sector data for data tracks.",
				std::regex("^(true|false)$"),
				"boolean",
				false,
				std::optional<std::string>("false"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.store_raw_tracks = matches.at(0) == "true";
				}
			}));
			parsers.push_back(parser::Parser({
				"audio-file-format",
				"Specify file format for audio tracks.",
				std::regex("^(bin|wav)$"),
				"bin|wav",
				false,
				std::optional<std::string>("wav"),
				1,
				1,
				[&](const std::vector<std::string>& matches) -> void {
					options.audio_format = matches.at(0);
				}
			}));
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
			(void)tracks;
		}

		auto get_track_tag(
			disc::TrackType type,
			bool_t store_raw_tracks
		) -> std::string {
			if (type == disc::TrackType::AUDIO_2_CHANNELS) {
				return "AUDIO";
			}
			if (type == disc::TrackType::AUDIO_4_CHANNELS) {
				return "AUDIO";
			}
			if (type == disc::TrackType::DATA_MODE0) {
				return store_raw_tracks ? "MODE1/2352" : "MODE1/2352";
			}
			if (type == disc::TrackType::DATA_MODE1) {
				return store_raw_tracks ? "MODE1/2352" : "MODE1/2048";
			}
			if (type == disc::TrackType::DATA_MODE2) {
				return store_raw_tracks ? "MODE2/2352" : "MODE2/2336";
			}
			if (type == disc::TrackType::DATA_MODE2_FORM1) {
				return store_raw_tracks ? "MODE2/2352" : "MODE2/2048";
			}
			if (type == disc::TrackType::DATA_MODE2_FORM2) {
				return store_raw_tracks ? "MODE2/2352" : "MODE2/2324";
			}
			OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
		}
	}
	}

	auto cue(
		const std::vector<std::string>& arguments,
		const command::Detail& detail
	) -> void {
		auto options = internal::parse_options(arguments);
		auto handle = detail.get_handle(options.drive);
		auto drive = drive::create_drive(handle, detail.ioctl);
		auto drive_info = drive.read_drive_info();
		drive_info.print();
		auto disc_info = drive.read_disc_info();
		disc_info.print();
		if (!options.read_correction) {
			options.read_correction = drive_info.read_offset_correction;
		}
		auto tracks = disc::get_disc_tracks(disc_info, options.track_numbers);
		internal::assert_image_compatibility(tracks);
		// TODO: Handle merge_tracks and audio_format option.
		for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
			auto& track = tracks.at(track_index);
			auto extracted_sectors_vector = copier::read_track(drive, track, options);
			auto bad_sector_indices = copier::get_bad_sector_indices(extracted_sectors_vector, track.first_sector_absolute);
			copier::log_bad_sector_indices(drive, track, bad_sector_indices);
			if (disc::is_data_track(track.type)) {
				auto bin_path = copier::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.bin", track.number));
				auto sector_data_offset = options.store_raw_tracks ? 0 : disc::get_user_data_offset(track.type);
				auto sector_data_length = options.store_raw_tracks ? cd::SECTOR_LENGTH : disc::get_user_data_length(track.type);
				copier::write_sector_data_to_file(extracted_sectors_vector, bin_path, sector_data_offset, sector_data_length);
			} else {
				auto extension = options.audio_format == "wav" ? "wav" : "bin";
				auto bin_path = copier::get_absolute_path_with_extension(options.path.value_or(""), std::format("{:0>2}.{}", track.number, extension));
				copier::write_sector_data_to_file(extracted_sectors_vector, bin_path, 0, cd::SECTOR_LENGTH);
			}
		}
		auto cue_path = copier::get_absolute_path_with_extension(options.path.value_or(""), std::format(".cue"));
		auto cue_handle = std::fopen(cue_path.c_str(), "wb+");
		if (cue_handle == nullptr) {
			OVERDRIVE_THROW(exceptions::IOOpenException(cue_path));
		}
		try {
			auto offset = size_t(0);
			for (auto track_index = size_t(0); track_index < tracks.size(); track_index += 1) {
				auto& track = tracks.at(track_index);
				auto is_audio_track = disc::is_audio_track(track.type);
				auto extension = is_audio_track && options.audio_format == "wav" ? "wav" : "bin";
				auto file_tag = is_audio_track && options.audio_format == "wav" ? "WAVE" : "BINARY";
				auto track_tag = internal::get_track_tag(track.type, options.store_raw_tracks);
				if (std::fprintf(cue_handle, "%s\n", std::format("FILE {}.{:0>2}.{} {}", "image", track.number, extension, file_tag).c_str()) < 0) {
					OVERDRIVE_THROW(exceptions::IOWriteException(cue_path));
				}
				if (std::fprintf(cue_handle, "%s\n", std::format("\tTRACK {:0>2} {}", track_index + 1, track_tag).c_str()) < 0) {
					OVERDRIVE_THROW(exceptions::IOWriteException(cue_path));
				}
				// TODO: Get pregap.
				if (std::fprintf(cue_handle, "%s\n", std::format("\t\tPREGAP {:0>2}:{:0>2}:{:0>2}", 0, 0, 0).c_str()) < 0) {
					OVERDRIVE_THROW(exceptions::IOWriteException(cue_path));
				}
				if (std::fprintf(cue_handle, "%s\n", std::format("\t\tINDEX {:0>2} {:0>2}:{:0>2}:{:0>2}", 1, 0, 0, 0).c_str()) < 0) {
					OVERDRIVE_THROW(exceptions::IOWriteException(cue_path));
				}
				// TODO: Increase offset if merge-tracks is true.
			}
		} catch (...) {
			std::fclose(cue_handle);
			throw;
		}
		std::fclose(cue_handle);
	};
}
