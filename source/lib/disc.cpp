#include "disc.h"

#include <algorithm>
#include "cdrom.h"
#include "cdxa.h"
#include "exceptions.h"

namespace overdrive {
namespace disc {
	auto is_audio_track(
		TrackType type
	) -> bool_t {
		if (type == TrackType::AUDIO_2_CHANNELS) {
			return true;
		}
		if (type == TrackType::AUDIO_4_CHANNELS) {
			return true;
		}
		if (type == TrackType::DATA_MODE0) {
			return false;
		}
		if (type == TrackType::DATA_MODE1) {
			return false;
		}
		if (type == TrackType::DATA_MODE2) {
			return false;
		}
		if (type == TrackType::DATA_MODE2_FORM1) {
			return false;
		}
		if (type == TrackType::DATA_MODE2_FORM2) {
			return false;
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto is_data_track(
		TrackType type
	) -> bool_t {
		if (type == TrackType::AUDIO_2_CHANNELS) {
			return false;
		}
		if (type == TrackType::AUDIO_4_CHANNELS) {
			return false;
		}
		if (type == TrackType::DATA_MODE0) {
			return true;
		}
		if (type == TrackType::DATA_MODE1) {
			return true;
		}
		if (type == TrackType::DATA_MODE2) {
			return true;
		}
		if (type == TrackType::DATA_MODE2_FORM1) {
			return true;
		}
		if (type == TrackType::DATA_MODE2_FORM2) {
			return true;
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto get_user_data_length(
		TrackType type
	) -> size_t {
		if (type == TrackType::DATA_MODE1) {
			return cdrom::MODE1_DATA_LENGTH;
		}
		if (type == TrackType::DATA_MODE2) {
			return cdrom::MODE2_DATA_LENGTH;
		}
		if (type == TrackType::DATA_MODE2_FORM1) {
			return cdxa::MODE2_FORM1_DATA_LENGTH;
		}
		if (type == TrackType::DATA_MODE2_FORM2) {
			return cdxa::MODE2_FORM2_DATA_LENGTH;
		}
		OVERDRIVE_THROW(exceptions::MissingValueException("user data length"));
	}

	auto truncate_disc(
		DiscInfo disc,
		size_t tracks
	) -> DiscInfo {
		auto tracks_left = tracks;
		for (auto session_index = size_t(0); session_index < disc.sessions.size(); session_index += 1) {
			auto& session = disc.sessions.at(session_index);
			auto size = std::min<size_t>(session.tracks.size(), tracks_left);
			for (auto track_index = size; track_index < session.tracks.size(); track_index += 1) {
				auto& track = session.tracks.at(track_index);
				session.length_sectors -= track.length_sectors;
				disc.length_sectors -= track.length_sectors;
			}
			session.tracks.resize(size);
			tracks_left -= size;
			if (size == 0) {
				disc.sessions.resize(session_index);
				break;
			}
		}
		return disc;
	}
}
}
