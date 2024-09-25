#pragma once

#include "cd.h"
#include "../type.h"

namespace cdrom {
	using namespace type;

	const auto SYNC_LENGTH = size_t(12);
	const auto HEADER_LENGTH = size_t(4);
	const auto SYNC_HEADER_LENGTH = size_t(SYNC_LENGTH + HEADER_LENGTH);
	const auto EDC_LENGTH = size_t(4);
	const auto PAD_LENGTH = size_t(8);
	const auto P_PARITY_LENGTH = size_t(172);
	const auto Q_PARITY_LENGTH = size_t(104);
	const auto ECC_LENGTH = size_t(P_PARITY_LENGTH + Q_PARITY_LENGTH);
	const auto EDC_PAD_ECC_LENGTH = size_t(EDC_LENGTH + PAD_LENGTH + ECC_LENGTH);
	const auto MODE1_DATA_LENGTH = size_t(cd::SECTOR_LENGTH - SYNC_HEADER_LENGTH - EDC_PAD_ECC_LENGTH);
	const auto MODE2_DATA_LENGTH = size_t(cd::SECTOR_LENGTH - SYNC_HEADER_LENGTH);

	#pragma pack(push, 1)

	#pragma pack(pop)
}
