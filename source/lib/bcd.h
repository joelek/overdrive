#pragma once

#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace bcd {
	using namespace shared;

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
