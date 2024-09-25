#pragma once

#include "cd.h"
#include "cdrom.h"
#include "../type.h"

namespace cdxa {
	using namespace type;

	const auto SUBHEADER_LENGTH = size_t(4);
	const auto MODE2_FORM1_DATA_LENGTH = size_t(cd::SECTOR_LENGTH - cdrom::SYNC_HEADER_LENGTH - SUBHEADER_LENGTH - SUBHEADER_LENGTH - cdrom::EDC_LENGTH - cdrom::ECC_LENGTH);
	const auto MODE2_FORM2_DATA_LENGTH = size_t(cd::SECTOR_LENGTH - cdrom::SYNC_HEADER_LENGTH - SUBHEADER_LENGTH - SUBHEADER_LENGTH - cdrom::EDC_LENGTH);

	#pragma pack(push, 1)

	#pragma pack(pop)
}
