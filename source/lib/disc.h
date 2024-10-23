#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>
#include "cdb.h"
#include "shared.h"

namespace overdrive {
namespace disc {
	using namespace shared;

	namespace TrackType {
		using type = ui08_t;

		const auto AUDIO_2_CHANNELS = type(0x00);
		const auto AUDIO_4_CHANNELS = type(0x01);
		const auto DATA_MODE0 = type(0x02);
		const auto DATA_MODE1 = type(0x03);
		const auto DATA_MODE2 = type(0x04);
		const auto DATA_MODE2_FORM1 = type(0x05);
		const auto DATA_MODE2_FORM2 = type(0x06);

		auto name(
			type value
		) -> const std::string&;
	}

	class DriveInfo {
		public:

		std::string vendor;
		std::string product;
		std::optional<size_t> sector_data_offset;
		std::optional<size_t> subchannels_data_offset;
		std::optional<size_t> c2_data_offset;
		size_t buffer_size;
		bool_t supports_accurate_stream;
		bool_t supports_c2_error_reporting;
		std::optional<si_t> read_offset_correction;

		auto print(
		) const -> void;

		protected:
	};

	class TrackInfo {
		public:

		size_t number;
		TrackType::type type;
		size_t first_sector_absolute;
		size_t last_sector_absolute;
		size_t length_sectors;

		auto print(
		) const -> void;

		protected:
	};

	class PointInfo {
		public:

		cdb::ReadTOCResponseFullTOCEntry entry;

		auto print(
		) const -> void;

		protected:
	};

	class SessionInfo {
		public:

		size_t number;
		cdb::SessionType::type type;
		std::vector<TrackInfo> tracks;
		std::vector<PointInfo> points;
		size_t lead_in_length_sectors;
		size_t pregap_sectors;
		size_t length_sectors;
		size_t lead_out_length_sectors;

		auto print(
		) const -> void;

		protected:
	};

	class DiscInfo {
		public:

		std::vector<SessionInfo> sessions;
		size_t length_sectors;

		auto print(
		) const -> void;

		protected:
	};

	auto is_audio_track(
		TrackType::type type
	) -> bool_t;

	auto is_data_track(
		TrackType::type type
	) -> bool_t;

	auto get_user_data_offset(
		TrackType::type type
	) -> size_t;

	auto get_user_data_length(
		TrackType::type type
	) -> size_t;

	auto truncate_disc(
		DiscInfo disc,
		size_t tracks
	) -> DiscInfo;

	auto get_disc_tracks(
		const DiscInfo& disc_info,
		const std::optional<std::set<size_t>>& track_numbers = std::nullopt
	) -> std::vector<TrackInfo>;

	auto get_disc_points(
		const DiscInfo& disc_info
	) -> std::vector<PointInfo>;
}
}
