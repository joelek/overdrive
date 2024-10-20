#include "disc.h"

#include <algorithm>
#include <format>
#include "cdrom.h"
#include "cdxa.h"
#include "enums.h"
#include "exceptions.h"
#include "string.h"

namespace overdrive {
namespace disc {
	auto DriveInfo::print(
	) const -> void {
		fprintf(stderr, "%s\n", std::format("Drive vendor: \"{}\"", string::trim(this->vendor)).c_str());
		fprintf(stderr, "%s\n", std::format("Drive product: \"{}\"", string::trim(this->product)).c_str());
		fprintf(stderr, "%s\n", std::format("Drive sector data offset: {}", this->sector_data_offset ? std::format("{}", this->sector_data_offset.value()) : "unknown").c_str());
		fprintf(stderr, "%s\n", std::format("Drive subchannels data offset: {}", this->subchannels_data_offset ? std::format("{}", this->subchannels_data_offset.value()) : "unknown").c_str());
		fprintf(stderr, "%s\n", std::format("Drive c2 data offset: {}", this->c2_data_offset ? std::format("{}", this->c2_data_offset.value()) : "unknown").c_str());
		fprintf(stderr, "%s\n", std::format("Drive buffer size [bytes]: {}", this->buffer_size).c_str());
		fprintf(stderr, "%s\n", std::format("Drive supports accurate stream: {}", this->supports_accurate_stream).c_str());
		fprintf(stderr, "%s\n", std::format("Drive supports c2 error reporting: {}", this->supports_c2_error_reporting).c_str());
		fprintf(stderr, "%s\n", std::format("Drive read offset correction [samples]: {}", this->read_offset_correction ? std::format("{}", this->read_offset_correction.value()) : "unknown").c_str());
	}

	auto TrackInfo::print(
	) const -> void {
		fprintf(stderr, "%s\n", std::format("\t\tTrack number: {}", this->number).c_str());
		fprintf(stderr, "%s\n", std::format("\t\tTrack type: {}", enums::TrackType(this->type)).c_str());
		fprintf(stderr, "%s\n", std::format("\t\tTrack first sector: {}", this->first_sector_absolute).c_str());
		fprintf(stderr, "%s\n", std::format("\t\tTrack last sector: {}", this->last_sector_absolute).c_str());
		fprintf(stderr, "%s\n", std::format("\t\tTrack length [sectors]: {}", this->length_sectors).c_str());
	}

	auto PointInfo::print(
	) const -> void {
		fprintf(stderr, "%s\n", std::format("\t\tPoint number: {:0>2X}", this->entry.point).c_str());
	}

	auto SessionInfo::print(
	) const -> void {
		fprintf(stderr, "%s\n", std::format("\tSession number: {}", this->number).c_str());
		fprintf(stderr, "%s\n", std::format("\tSession type: {}", enums::SessionType(this->type)).c_str());
		fprintf(stderr, "%s\n", std::format("\tSession tracks: {}", this->tracks.size()).c_str());
		fprintf(stderr, "%s\n", std::format("\tSession lead-in length [sectors]: {}", this->lead_in_length_sectors).c_str());
		fprintf(stderr, "%s\n", std::format("\tSession pregap length [sectors]: {}", this->pregap_sectors).c_str());
		fprintf(stderr, "%s\n", std::format("\tSession length [sectors]: {}", this->length_sectors).c_str());
		fprintf(stderr, "%s\n", std::format("\tSession lead-out length [sectors]: {}", this->lead_out_length_sectors).c_str());
		for (auto track_index = size_t(0); track_index < this->tracks.size(); track_index += 1) {
			auto& track = this->tracks.at(track_index);
			track.print();
		}
		fprintf(stderr, "%s\n", std::format("\tSession points: {}", this->points.size()).c_str());
		for (auto point_index = size_t(0); point_index < this->points.size(); point_index += 1) {
			auto& point = this->points.at(point_index);
			point.print();
		}
	}

	auto DiscInfo::print(
	) const -> void {
		fprintf(stderr, "%s\n", std::format("Disc sessions: {}", this->sessions.size()).c_str());
		fprintf(stderr, "%s\n", std::format("Disc length [sectors]: {}", this->length_sectors).c_str());
		for (auto session_index = size_t(0); session_index < this->sessions.size(); session_index += 1) {
			auto& session = this->sessions.at(session_index);
			session.print();
		}
	}

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

	auto get_user_data_offset(
		TrackType type
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
