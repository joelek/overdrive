#pragma once

#include "type.h"

namespace idiv {
	using namespace type;

	auto ceil(
		si_t n,
		si_t d
	) -> si_t;

	auto floor(
		si_t n,
		si_t d
	) -> si_t;

	auto round(
		si_t n,
		si_t d
	) -> si_t;
}
