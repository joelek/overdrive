#pragma once

#include "../type.h"

namespace cd {
	using namespace type;

	const auto SECTOR_LENGTH = size_t(2352);
	const auto SECTORS_PER_SECOND = size_t(75);
	const auto SUBCHANNEL_COUNT = size_t(8);
	const auto SUBCHANNEL_LENGTH = size_t(12);
	const auto SUBCHANNEL_P_INDEX = size_t(0);
	const auto SUBCHANNEL_Q_INDEX = size_t(1);
	const auto SUBCHANNEL_R_INDEX = size_t(2);
	const auto SUBCHANNEL_S_INDEX = size_t(3);
	const auto SUBCHANNEL_T_INDEX = size_t(4);
	const auto SUBCHANNEL_U_INDEX = size_t(5);
	const auto SUBCHANNEL_V_INDEX = size_t(6);
	const auto SUBCHANNEL_W_INDEX = size_t(7);
	const auto SUBCHANNELS_LENGTH = size_t(SUBCHANNEL_COUNT * SUBCHANNEL_LENGTH);
	const auto C2_ERROR_POINTERS_LENGTH = size_t(SECTOR_LENGTH / 8);

	#pragma pack(push, 1)

	enum class SessionType: ui08_t {
		CDDA_OR_CDROM = 0x00,
		CDI = 0x10,
		CDXA_OR_DDCD = 0x20
	};

	static_assert(sizeof(SessionType) == 1);

	#pragma pack(pop)
}
