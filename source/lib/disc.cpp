#include "disc.h"

#include <algorithm>
#include <format>
#include <map>
#include "cdrom.h"
#include "cdxa.h"
#include "exceptions.h"
#include "string.h"

namespace overdrive {
namespace disc {
	auto TrackType::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ AUDIO_2_CHANNELS, "AUDIO_2_CHANNELS" },
			{ AUDIO_4_CHANNELS, "AUDIO_4_CHANNELS" },
			{ DATA_MODE0, "DATA_MODE0" },
			{ DATA_MODE1, "DATA_MODE1" },
			{ DATA_MODE2, "DATA_MODE2" },
			{ DATA_MODE2_FORM1, "DATA_MODE2_FORM1" },
			{ DATA_MODE2_FORM2, "DATA_MODE2_FORM2" },
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	auto DriveInfo::print(
	) const -> void {
		OVERDRIVE_LOG("Drive vendor: \"{}\"", string::trim(this->vendor));
		OVERDRIVE_LOG("Drive product: \"{}\"", string::trim(this->product));
		OVERDRIVE_LOG("Drive sector data offset: {}", this->sector_data_offset ? std::format("{}", this->sector_data_offset.value()) : "unknown");
		OVERDRIVE_LOG("Drive subchannels data offset: {}", this->subchannels_data_offset ? std::format("{}", this->subchannels_data_offset.value()) : "unknown");
		OVERDRIVE_LOG("Drive c2 data offset: {}", this->c2_data_offset ? std::format("{}", this->c2_data_offset.value()) : "unknown");
		OVERDRIVE_LOG("Drive buffer size [bytes]: {}", this->buffer_size);
		OVERDRIVE_LOG("Drive supports accurate stream: {}", this->supports_accurate_stream);
		OVERDRIVE_LOG("Drive supports c2 error reporting: {}", this->supports_c2_error_reporting);
		OVERDRIVE_LOG("Drive read offset correction [samples]: {}", this->read_offset_correction ? std::format("{}", this->read_offset_correction.value()) : "unknown");
	}

	auto TrackInfo::print(
	) const -> void {
		OVERDRIVE_LOG("\t\tTrack number: {}", this->number);
		OVERDRIVE_LOG("\t\tTrack type: {}", disc::TrackType::name(this->type));
		OVERDRIVE_LOG("\t\tTrack first sector: {}", this->first_sector_absolute);
		OVERDRIVE_LOG("\t\tTrack last sector: {}", this->last_sector_absolute);
		OVERDRIVE_LOG("\t\tTrack length [sectors]: {}", this->length_sectors);
	}

	auto PointInfo::print(
	) const -> void {
		OVERDRIVE_LOG("\t\tPoint number: {:0>2X}", this->entry.point);
	}

	auto SessionInfo::print(
	) const -> void {
		OVERDRIVE_LOG("\tSession number: {}", this->number);
		OVERDRIVE_LOG("\tSession type: {}", cdb::SessionType::name(this->type));
		OVERDRIVE_LOG("\tSession tracks: {}", this->tracks.size());
		OVERDRIVE_LOG("\tSession lead-in length [sectors]: {}", this->lead_in_length_sectors);
		OVERDRIVE_LOG("\tSession pregap length [sectors]: {}", this->pregap_sectors);
		OVERDRIVE_LOG("\tSession length [sectors]: {}", this->length_sectors);
		OVERDRIVE_LOG("\tSession lead-out length [sectors]: {}", this->lead_out_length_sectors);
		for (auto track_index = size_t(0); track_index < this->tracks.size(); track_index += 1) {
			auto& track = this->tracks.at(track_index);
			track.print();
		}
		OVERDRIVE_LOG("\tSession points: {}", this->points.size());
		for (auto point_index = size_t(0); point_index < this->points.size(); point_index += 1) {
			auto& point = this->points.at(point_index);
			point.print();
		}
	}

	auto DiscInfo::print(
	) const -> void {
		OVERDRIVE_LOG("Disc sessions: {}", this->sessions.size());
		OVERDRIVE_LOG("Disc length [sectors]: {}", this->length_sectors);
		for (auto session_index = size_t(0); session_index < this->sessions.size(); session_index += 1) {
			auto& session = this->sessions.at(session_index);
			session.print();
		}
	}

	auto is_audio_track(
		TrackType::type type
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
		TrackType::type type
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

	auto get_user_data_offset(
		TrackType::type type
	) -> size_t {
		if (type == TrackType::DATA_MODE1) {
			return offsetof(cdrom::Mode1Sector, user_data);
		}
		if (type == TrackType::DATA_MODE2) {
			return offsetof(cdrom::Mode2Sector, user_data);
		}
		if (type == TrackType::DATA_MODE2_FORM1) {
			return offsetof(cdxa::Mode2Form1Sector, user_data);
		}
		if (type == TrackType::DATA_MODE2_FORM2) {
			return offsetof(cdxa::Mode2Form2Sector, user_data);
		}
		OVERDRIVE_THROW(exceptions::MissingValueException("user data offset"));
	}

	auto get_user_data_length(
		TrackType::type type
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

	auto get_disc_tracks(
		const DiscInfo& disc_info,
		const std::optional<std::set<size_t>>& track_numbers
	) -> std::vector<TrackInfo> {
		auto tracks = std::vector<TrackInfo>();
		for (auto session_index = size_t(0); session_index < disc_info.sessions.size(); session_index += 1) {
			auto& session = disc_info.sessions.at(session_index);
			for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
				auto& track = session.tracks.at(track_index);
				if (track_numbers && !track_numbers->contains(track.number)) {
					continue;
				}
				tracks.push_back(track);
			}
		}
		return tracks;
	}

	auto get_disc_points(
		const DiscInfo& disc_info
	) -> std::vector<PointInfo> {
		auto points = std::vector<PointInfo>();
		for (auto session_index = size_t(0); session_index < disc_info.sessions.size(); session_index += 1) {
			auto& session = disc_info.sessions.at(session_index);
			for (auto point_index = size_t(0); point_index < session.points.size(); point_index += 1) {
				auto& point = session.points.at(point_index);
				points.push_back(point);
			}
		}
		return points;
	}
}
}
