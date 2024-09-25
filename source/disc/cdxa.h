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

	#pragma pack(pop)
}
