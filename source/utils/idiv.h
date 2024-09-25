#pragma once

#include "../type.h"

namespace idiv {
	using namespace type;

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
