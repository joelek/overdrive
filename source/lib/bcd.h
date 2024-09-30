#pragma once

#include "cd.h"
#include "type.h"

namespace overdrive {
namespace bcd {
	using namespace type;

	auto decode(
		ui08_t byte
	) -> ui08_t;

	auto encode(
		ui08_t byte
	) -> ui08_t;

	auto decode_address(
		const cd::SectorAddress& address
	) -> cd::SectorAddress;

	auto encode_address(
		const cd::SectorAddress& address
	) -> cd::SectorAddress;
}
}
