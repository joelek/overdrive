#include "cdb.h"

#include "../lib/exceptions.h"
#include "../utils/byteswap.h"

namespace scsi {
namespace cdb {
	auto get_session_type(
		const ReadTOCResponseFullTOC& toc
	) -> SessionType {
		auto toc_length = utils::byteswap::byteswap16(toc.header.data_length_be);
		auto track_count = (toc_length - sizeof(toc.header.data_length_be)) / sizeof(ReadTOCResponseFullTOCEntry);
		for (auto track_index = size_t(0); track_index < track_count; track_index += 1) {
			auto &track = toc.entries[track_index];
			if (track.point == 0xA0) {
				return static_cast<SessionType>(track.paddress.s);
			}
		}
		throw overdrive::exceptions::MissingValueException("point 0xA0");
	}

	auto validate_toc(
		const ReadTOCResponseNormalTOC& toc
	) -> void {
		auto length = utils::byteswap::byteswap16(toc.header.data_length_be);
		auto min_length = sizeof(toc.header) - sizeof(toc.header.data_length_be);
		auto max_length = sizeof(toc) - sizeof(toc.header.data_length_be);
		if (length < min_length || length > max_length) {
			throw overdrive::exceptions::InvalidValueException("length", length, min_length, max_length);
		}
		if (toc.header.first_track_or_session_number < 1 || toc.header.first_track_or_session_number > 99) {
			throw overdrive::exceptions::InvalidValueException("first track", toc.header.first_track_or_session_number, 1, 99);
		}
		if (toc.header.last_track_or_session_number < 1 || toc.header.last_track_or_session_number > 99) {
			throw overdrive::exceptions::InvalidValueException("last track", toc.header.last_track_or_session_number, 1, 99);
		}
		auto number_of_tracks = toc.header.last_track_or_session_number - toc.header.first_track_or_session_number + 1;
		if (number_of_tracks < 1 || number_of_tracks > 99) {
			throw overdrive::exceptions::InvalidValueException("number of tracks", number_of_tracks, 1, 99);
		}
		auto computed_length = min_length + (number_of_tracks + 1) * sizeof(toc.entries[0]);
		if (computed_length != length) {
			throw overdrive::exceptions::InvalidValueException("computed length", computed_length, length, length);
		}
	}
}
}
