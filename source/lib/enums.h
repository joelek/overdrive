#pragma once

#include <string>
#include "cdb.h"
#include "type.h"

namespace overdrive {
namespace enums {
	using namespace type;

	auto SessionType(
		cdb::SessionType session_type
	) -> const std::string&;
}
}
