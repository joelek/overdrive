#include "mds.h"

#include "exceptions.h"

namespace overdrive {
namespace mds {
	auto get_track_mode(
		disc::TrackType track_type
	) -> TrackMode {
		if (track_type == disc::TrackType::AUDIO_2_CHANNELS) {
			return TrackMode::AUDIO;
		}
		if (track_type == disc::TrackType::AUDIO_4_CHANNELS) {
			return TrackMode::AUDIO;
		}
		if (track_type == disc::TrackType::DATA_MODE0) {
			return TrackMode::NONE;
		}
		if (track_type == disc::TrackType::DATA_MODE1) {
			return TrackMode::MODE1;
		}
		if (track_type == disc::TrackType::DATA_MODE2) {
			return TrackMode::MODE2;
		}
		if (track_type == disc::TrackType::DATA_MODE2_FORM1) {
			return TrackMode::MODE2_FORM1;
		}
		if (track_type == disc::TrackType::DATA_MODE2_FORM2) {
			return TrackMode::MODE2_FORM2;
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}
}
}
