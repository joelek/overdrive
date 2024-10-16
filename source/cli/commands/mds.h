#pragma once

#include <string>
#include <vector>
#include "../../lib/overdrive.h"

using namespace overdrive;
using namespace shared;

namespace commands {
	auto mds(
		const std::vector<std::string>& arguments,
		const command::Detail& detail
	) -> void;
}
