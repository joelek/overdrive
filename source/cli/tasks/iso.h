#pragma once

#include <string>
#include <vector>
#include "../../lib/overdrive.h"

using namespace overdrive;
using namespace shared;

namespace tasks {
	auto iso(
		const std::vector<std::string>& arguments
	) -> void;
}
