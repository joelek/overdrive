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
			OVERDRIVE_THROW(exceptions::MissingValueException("name"));
		}
		return iterator->second;
	}

	auto TrackType(
		disc::TrackType value
	) -> const std::string& {
		static const auto names = std::map<disc::TrackType, std::string>({
			{ disc::TrackType::AUDIO_2_CHANNELS, "AUDIO_2_CHANNELS" },
			{ disc::TrackType::AUDIO_4_CHANNELS, "AUDIO_4_CHANNELS" },
			{ disc::TrackType::DATA_MODE0, "DATA_MODE0" },
			{ disc::TrackType::DATA_MODE1, "DATA_MODE1" },
			{ disc::TrackType::DATA_MODE2, "DATA_MODE2" },
			{ disc::TrackType::DATA_MODE2_FORM1, "DATA_MODE2_FORM1" },
			{ disc::TrackType::DATA_MODE2_FORM2, "DATA_MODE2_FORM2" },
		});
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			OVERDRIVE_THROW(exceptions::MissingValueException("name"));
		}
		return iterator->second;
	}

	auto SensePage(
		cdb::SensePage value
	) -> const std::string& {
		static const auto names = std::map<cdb::SensePage, std::string>({
			{ cdb::SensePage::CACHING_MODE_PAGE, "CACHING_MODE_PAGE" },
			{ cdb::SensePage::CAPABILITIES_AND_MECHANICAL_STATUS_PAGE, "CAPABILITIES_AND_MECHANICAL_STATUS_PAGE" },
			{ cdb::SensePage::READ_WRITE_ERROR_RECOVERY_MODE_PAGE, "READ_WRITE_ERROR_RECOVERY_MODE_PAGE" }
		});
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			OVERDRIVE_THROW(exceptions::MissingValueException("name"));
		}
		return iterator->second;
	}
}
}
