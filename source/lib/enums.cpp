#include "enums.h"

#include <map>
#include "exceptions.h"

namespace overdrive {
namespace enums {
	auto SessionType(
		cdb::SessionType session_type
	) -> const std::string& {
		static const auto names = std::map<cdb::SessionType, std::string>({
			{ cdb::SessionType::CDDA_OR_CDROM, "CDDA_OR_CDROM" },
			{ cdb::SessionType::CDI, "CDI" },
			{ cdb::SessionType::CDXA_OR_DDCD, "CDXA_OR_DDCD" }
		});
		auto iterator = names.find(session_type);
		if (iterator == names.end()) {
			throw exceptions::MissingValueException("name");
		}
		return iterator->second;
	}
}
}
