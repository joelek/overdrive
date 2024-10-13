#pragma once

#include "shared.h"

namespace overdrive {
namespace endian {
	using namespace shared;

	auto is_system_big_endian()
	-> bool_t;

	auto is_system_little_endian()
	-> bool_t;
}
}
