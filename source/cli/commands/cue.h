#pragma once

#include <string>
#include <vector>
#include "../../lib/overdrive.h"

using namespace overdrive;
using namespace shared;

namespace commands {
	auto cue(
		const std::vector<std::string>& arguments
	) -> void;
}
