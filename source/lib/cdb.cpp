#include "cdb.h"

#include <map>
#include "byteswap.h"
#include "exceptions.h"

namespace overdrive {
namespace cdb {
	auto SessionType::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ CDDA_OR_CDROM, "CDDA_OR_CDROM" },
			{ CDI, "CDI" },
			{ CDXA_OR_DDCD, "CDXA_OR_DDCD" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	auto SensePage::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ CACHING_MODE_PAGE, "CACHING_MODE_PAGE" },
			{ CAPABILITIES_AND_MECHANICAL_STATUS_PAGE, "CAPABILITIES_AND_MECHANICAL_STATUS_PAGE" },
			{ READ_WRITE_ERROR_RECOVERY_MODE_PAGE, "READ_WRITE_ERROR_RECOVERY_MODE_PAGE" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	auto get_session_type(
		const ReadTOCResponseFullTOC& toc
	) -> SessionType::type {
		auto toc_length = byteswap::byteswap16_on_little_endian_systems(toc.header.data_length_be);
		auto track_count = (toc_length - sizeof(toc.header.data_length_be)) / sizeof(ReadTOCResponseFullTOCEntry);
		for (auto track_index = size_t(0); track_index < track_count; track_index += 1) {
			auto& track = toc.entries[track_index];
			if (track.adr == 1 && track.point == size_t(ReadTOCResponseFullTOCPoint::FIRST_TRACK_IN_SESSION)) {
				return track.paddress.s;
			}
		}
		OVERDRIVE_THROW(exceptions::MissingValueException("first track point"));
	}

	auto validate_session_info_toc(
		const ReadTOCResponseSessionInfoTOC& toc
	) -> size_t {
		auto length = byteswap::byteswap16_on_little_endian_systems(toc.header.data_length_be);
		auto min_length = sizeof(toc.header) - sizeof(toc.header.data_length_be);
		auto max_length = sizeof(toc) - sizeof(toc.header.data_length_be);
		if (length < min_length || length > max_length) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("length", length, min_length, max_length));
		}
		if (toc.header.first_track_or_session_number < 1 || toc.header.first_track_or_session_number > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("first session", toc.header.first_track_or_session_number, 1, 99));
		}
		if (toc.header.last_track_or_session_number < 1 || toc.header.last_track_or_session_number > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("last session", toc.header.last_track_or_session_number, 1, 99));
		}
		auto number_of_sessions = size_t(toc.header.last_track_or_session_number) - size_t(toc.header.first_track_or_session_number) + 1;
		if (number_of_sessions < 1 || number_of_sessions > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("number of sessions", number_of_sessions, 1, 99));
		}
		auto number_of_entries = (length - min_length) / sizeof(toc.entries[0]);
		if (number_of_entries != number_of_sessions) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("number of entries", number_of_entries, number_of_sessions, number_of_sessions));
		}
		auto computed_length = min_length + number_of_entries * sizeof(toc.entries[0]);
		if (computed_length != length) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("computed length", computed_length, length, length));
		}
		return number_of_entries;
	}

	auto validate_normal_toc(
		const ReadTOCResponseNormalTOC& toc
	) -> size_t {
		auto length = byteswap::byteswap16_on_little_endian_systems(toc.header.data_length_be);
		auto min_length = sizeof(toc.header) - sizeof(toc.header.data_length_be);
		auto max_length = sizeof(toc) - sizeof(toc.header.data_length_be);
		if (length < min_length || length > max_length) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("length", length, min_length, max_length));
		}
		if (toc.header.first_track_or_session_number < 1 || toc.header.first_track_or_session_number > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("first track", toc.header.first_track_or_session_number, 1, 99));
		}
		if (toc.header.last_track_or_session_number < 1 || toc.header.last_track_or_session_number > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("last track", toc.header.last_track_or_session_number, 1, 99));
		}
		auto number_of_tracks = size_t(toc.header.last_track_or_session_number) - size_t(toc.header.first_track_or_session_number) + 1;
		if (number_of_tracks < 1 || number_of_tracks > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("number of tracks", number_of_tracks, 1, 99));
		}
		auto number_of_entries = (length - min_length) / sizeof(toc.entries[0]);
		if (number_of_entries != number_of_tracks + 1) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("number of entries", number_of_entries, number_of_tracks + 1, number_of_tracks + 1));
		}
		auto computed_length = min_length + number_of_entries * sizeof(toc.entries[0]);
		if (computed_length != length) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("computed length", computed_length, length, length));
		}
		return number_of_entries;
	}

	auto validate_full_toc(
		const ReadTOCResponseFullTOC& toc
	) -> size_t {
		auto length = byteswap::byteswap16_on_little_endian_systems(toc.header.data_length_be);
		auto min_length = sizeof(toc.header) - sizeof(toc.header.data_length_be);
		auto max_length = sizeof(toc) - sizeof(toc.header.data_length_be);
		if (length < min_length || length > max_length) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("length", length, min_length, max_length));
		}
		if (toc.header.first_track_or_session_number < 1 || toc.header.first_track_or_session_number > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("first session", toc.header.first_track_or_session_number, 1, 99));
		}
		if (toc.header.last_track_or_session_number < 1 || toc.header.last_track_or_session_number > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("last session", toc.header.last_track_or_session_number, 1, 99));
		}
		auto number_of_sessions = size_t(toc.header.last_track_or_session_number) - size_t(toc.header.first_track_or_session_number) + 1;
		if (number_of_sessions < 1 || number_of_sessions > 99) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("number of sessions", number_of_sessions, 1, 99));
		}
		auto number_of_entires = (length - min_length) / sizeof(toc.entries[0]);
		auto computed_length = min_length + number_of_entires * sizeof(toc.entries[0]);
		if (computed_length != length) {
			OVERDRIVE_THROW(exceptions::InvalidValueException("computed length", computed_length, length, length));
		}
		return number_of_entires;
	}

	auto is_track_reference(
		const ReadTOCResponseFullTOCEntry& entry
	) -> bool_t {
		return entry.adr == 1 && entry.point >= size_t(ReadTOCResponseFullTOCPoint::FIRST_TRACK_REFERENCE) && entry.point <= size_t(ReadTOCResponseFullTOCPoint::LAST_TRACK_REFERENCE);
	}
}
}
