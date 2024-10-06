#pragma once

#include <string>
#include <vector>
#include "../../lib/overdrive.h"

using namespace overdrive;
using namespace type;

namespace commands {
	auto mds(
		const std::vector<std::string>& arguments
	) -> void;
}