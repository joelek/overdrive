#pragma once

#include "shared.h"

namespace overdrive {
namespace time {
	using namespace shared;

	auto get_duration_ms(
		si64_t start_ms
	) -> si64_t;

	auto get_time_ms(
	) -> si64_t;
}
}
