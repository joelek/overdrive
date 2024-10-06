#pragma once

#include <optional>
#include <string>
#include <vector>
#include "cdb.h"
#include "shared.h"

namespace overdrive {
namespace disc {
	using namespace shared;

	#pragma pack(push, 1)

	#pragma pack(pop)

	enum class TrackType {
		AUDIO_2_CHANNELS,
		AUDIO_4_CHANNELS,
		DATA_MODE0,
		DATA_MODE1,
		DATA_MODE2,
		DATA_MODE2_FORM1,
		DATA_MODE2_FORM2
	};

	class DriveInfo {
		public:

		std::string vendor;
		std::string product;
		size_t sector_data_offset;
		size_t subchannels_data_offset;
		size_t c2_data_offset;
		size_t buffer_size;
		bool_t supports_accurate_stream;
		bool_t supports_c2_error_reporting;
		std::optional<si_t> read_offset_correction;

		protected:
	};

	class TrackInfo {
		public:

		size_t number;
		TrackType type;
		size_t first_sector_absolute;
		size_t length_sectors;

		protected:
	};

	class SessionInfo {
		public:

		size_t number;
		cdb::SessionType type;
		std::vector<TrackInfo> tracks;
		size_t length_sectors;

		protected:
	};

	class DiscInfo {
		public:

		std::vector<SessionInfo> sessions;
		size_t length_sectors;

		protected:
	};

	auto is_audio_track(
		TrackType type
	) -> bool_t;

	auto is_data_track(
		TrackType type
	) -> bool_t;

	auto get_user_data_length(
		TrackType type
	) -> size_t;
}
}
