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
		throw overdrive::exceptions::MissingValueException();
	}
}
}
