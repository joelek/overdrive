#include "enums.h"

#include <map>
#include "exceptions.h"

namespace overdrive {
namespace enums {
	auto SessionType(
		cdb::SessionType value
	) -> const std::string& {
		static const auto names = std::map<cdb::SessionType, std::string>({
			{ cdb::SessionType::CDDA_OR_CDROM, "CDDA_OR_CDROM" },
			{ cdb::SessionType::CDI, "CDI" },
			{ cdb::SessionType::CDXA_OR_DDCD, "CDXA_OR_DDCD" }
		});
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			throw exceptions::MissingValueException("name");
		}
		return iterator->second;
	}

	auto TrackType(
		drive::TrackType value
	) -> const std::string& {
		static const auto names = std::map<drive::TrackType, std::string>({
			{ drive::TrackType::AUDIO_2_CHANNELS, "AUDIO_2_CHANNELS" },
			{ drive::TrackType::AUDIO_4_CHANNELS, "AUDIO_4_CHANNELS" },
			{ drive::TrackType::DATA_MODE0, "DATA_MODE0" },
			{ drive::TrackType::DATA_MODE1, "DATA_MODE1" },
			{ drive::TrackType::DATA_MODE2, "DATA_MODE2" },
			{ drive::TrackType::DATA_MODE2_FORM1, "DATA_MODE2_FORM1" },
			{ drive::TrackType::DATA_MODE2_FORM2, "DATA_MODE2_FORM2" },
		});
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			throw exceptions::MissingValueException("name");
		}
		return iterator->second;
	}
}
}
