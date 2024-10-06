#pragma once

#include <string>
#include "cdb.h"
#include "disc.h"
#include "shared.h"

namespace overdrive {
namespace enums {
	using namespace shared;

	auto SessionType(
		cdb::SessionType value
	) -> const std::string&;

	auto TrackType(
		disc::TrackType value
	) -> const std::string&;
}
}
