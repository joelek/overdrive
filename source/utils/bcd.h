#pragma once

#include "../type.h"

namespace utils {
namespace bcd {
	using namespace type;

	auto decode(
		ui08_t byte
	) -> ui08_t;

	auto encode(
		ui08_t byte
	) -> ui08_t;
}
}
