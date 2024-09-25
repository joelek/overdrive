#pragma once

#include "cd.h"
#include "cdrom.h"
#include "../type.h"

namespace cdxa {
	using namespace type;

	const auto SECTOR_LENGTH = size_t(cd::SECTOR_LENGTH);
	const auto SUBHEADER_LENGTH = size_t(4);
	const auto BASE_SECTOR_LENGTH = size_t(SECTOR_LENGTH);
	const auto BASE_SECTOR_DATA_LENGTH = size_t(BASE_SECTOR_LENGTH - cdrom::SYNC_HEADER_LENGTH - SUBHEADER_LENGTH - SUBHEADER_LENGTH);
	const auto MODE2_FORM1_SECTOR_LENGTH = size_t(BASE_SECTOR_LENGTH);
	const auto MODE2_FORM1_DATA_LENGTH = size_t(BASE_SECTOR_DATA_LENGTH - cdrom::EDC_LENGTH - cdrom::ECC_LENGTH);
	const auto MODE2_FORM2_SECTOR_LENGTH = size_t(BASE_SECTOR_LENGTH);
	const auto MODE2_FORM2_DATA_LENGTH = size_t(BASE_SECTOR_DATA_LENGTH - cdrom::EDC_LENGTH);

	#pragma pack(push, 1)

	struct Subheader {
		ui08_t file_number: 8;
		ui08_t channel_number: 8;
		ui08_t end_of_record: 1;
		ui08_t video_block: 1;
		ui08_t audio_block: 1;
		ui08_t data_block: 1;
		ui08_t trigger_block: 1;
		ui08_t form_2: 1;
		ui08_t real_time_block: 1;
		ui08_t end_of_file: 1;
		ui08_t coding_info: 8;
	};

	static_assert(sizeof(Subheader) == SUBHEADER_LENGTH);

	struct BaseSector {
		cdrom::SyncHeader header;
		Subheader header_1;
		Subheader header_2;
		byte_t data[BASE_SECTOR_DATA_LENGTH];
	};

	static_assert(sizeof(BaseSector) == BASE_SECTOR_LENGTH);

	struct Mode2Form1Sector {
		cdrom::SyncHeader header;
		Subheader header_1;
		Subheader header_2;
		byte_t user_data[MODE2_FORM1_DATA_LENGTH];
		byte_t edc[cdrom::EDC_LENGTH];
		byte_t p_parity[cdrom::P_PARITY_LENGTH];
		byte_t q_parity[cdrom::Q_PARITY_LENGTH];
	};

	static_assert(sizeof(Mode2Form1Sector) == MODE2_FORM1_SECTOR_LENGTH);

	struct Mode2Form2Sector {
		cdrom::SyncHeader header;
		Subheader header_1;
		Subheader header_2;
		byte_t user_data[MODE2_FORM2_DATA_LENGTH];
		byte_t optional_edc[cdrom::EDC_LENGTH];
	};

	static_assert(sizeof(Mode2Form2Sector) == MODE2_FORM2_SECTOR_LENGTH);

	union Sector {
		BaseSector base;
		Mode2Form1Sector mode2form1;
		Mode2Form2Sector mode2form2;
	};

	static_assert(sizeof(Sector) == SECTOR_LENGTH);

	#pragma pack(pop)
}
