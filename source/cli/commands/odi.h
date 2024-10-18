#pragma once

#include <string>
#include <vector>
#include "../../lib/overdrive.h"

using namespace overdrive;
using namespace shared;

namespace commands {
	auto odi(
		const std::vector<std::string>& arguments,
		const detail::Detail& detail
	) -> void;
}
