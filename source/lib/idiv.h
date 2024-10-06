#pragma once

#include "shared.h"

namespace overdrive {
namespace idiv {
	using namespace shared;

	auto ceil(
		si_t num,
		si_t den
	) -> si_t;

	auto floor(
		si_t num,
		si_t den
	) -> si_t;

	auto round(
		si_t num,
		si_t den
	) -> si_t;
}
}
