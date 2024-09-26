#include "cd.h"

#include "../lib/exceptions.h"

namespace discs {
namespace cd {
	auto get_sector_from_address(
		const SectorAddress& address
	) -> ui_t {
		auto sector = (address.m * MINUTES_PER_SECOND + address.s) * SECTORS_PER_SECOND + address.f;
		if (sector > MAX_SECTOR) {
			throw overdrive::exceptions::BadSectorException(sector);
		}
		return sector;
	}

	auto get_address_from_sector(
		ui_t sector
	) -> SectorAddress {
		if (sector > MAX_SECTOR) {
			throw overdrive::exceptions::BadSectorException(sector);
		}
		auto f = sector;
		auto s = sector / SECTORS_PER_SECOND;
		f -= s * SECTORS_PER_SECOND;
		auto m = s / MINUTES_PER_SECOND;
		s -= m * MINUTES_PER_SECOND;
		auto address = SectorAddress();
		address.m = m;
		address.s = s;
		address.f = f;
		return address;
	}
}
}
