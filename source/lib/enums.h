#pragma once

#include <string>
#include "cdb.h"
#include "drive.h"
#include "type.h"

namespace overdrive {
namespace enums {
	using namespace type;

	auto SessionType(
		cdb::SessionType value
	) -> const std::string&;

	auto TrackType(
		drive::TrackType value
	) -> const std::string&;
}
}
