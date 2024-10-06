#pragma once

#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace cdrom {
	using namespace shared;

	const auto SECTOR_LENGTH = size_t(cd::SECTOR_LENGTH);
	const auto SYNC_LENGTH = size_t(12);
	const auto HEADER_LENGTH = size_t(4);
	const auto SYNC_HEADER_LENGTH = size_t(SYNC_LENGTH + HEADER_LENGTH);
	const auto EDC_LENGTH = size_t(4);
	const auto PAD_LENGTH = size_t(8);
	const auto P_PARITY_LENGTH = size_t(172);
	const auto Q_PARITY_LENGTH = size_t(104);
	const auto ECC_LENGTH = size_t(P_PARITY_LENGTH + Q_PARITY_LENGTH);
	const auto EDC_PAD_ECC_LENGTH = size_t(EDC_LENGTH + PAD_LENGTH + ECC_LENGTH);
	const auto BASE_SECTOR_LENGTH = size_t(cd::SECTOR_LENGTH);
	const auto BASE_SECTOR_DATA_LENGTH = size_t(BASE_SECTOR_LENGTH - cdrom::SYNC_HEADER_LENGTH);
	const auto MODE1_SECTOR_LENGTH = size_t(BASE_SECTOR_LENGTH);
	const auto MODE1_DATA_LENGTH = size_t(BASE_SECTOR_DATA_LENGTH - EDC_PAD_ECC_LENGTH);
	const auto MODE2_SECTOR_LENGTH = size_t(BASE_SECTOR_LENGTH);
	const auto MODE2_DATA_LENGTH = size_t(BASE_SECTOR_DATA_LENGTH);

	#pragma pack(push, 1)

	struct SyncHeader {
		byte_t sync[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
		cd::SectorAddress absolute_address_bcd;
		ui08_t mode;
	};

	static_assert(sizeof(SyncHeader) == SYNC_HEADER_LENGTH);

	struct BaseSector {
		SyncHeader header;
		byte_t data[BASE_SECTOR_DATA_LENGTH];
	};

	static_assert(sizeof(BaseSector) == BASE_SECTOR_LENGTH);

	struct Mode1Sector {
		SyncHeader header;
		byte_t user_data[MODE1_DATA_LENGTH];
		byte_t edc[EDC_LENGTH];
		byte_t pad[PAD_LENGTH];
		byte_t ecc[ECC_LENGTH];
	};

	static_assert(sizeof(Mode1Sector) == MODE1_SECTOR_LENGTH);

	struct Mode2Sector {
		SyncHeader header;
		byte_t user_data[MODE2_DATA_LENGTH];
	};

	static_assert(sizeof(Mode2Sector) == MODE2_SECTOR_LENGTH);

	union Sector {
		BaseSector base;
		Mode1Sector mode1;
		Mode2Sector mode2;
	};

	static_assert(sizeof(Sector) == SECTOR_LENGTH);

	#pragma pack(pop)
}
}
