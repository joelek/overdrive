#pragma once

#include "../type.h"

namespace cd {
	using namespace type;

	#pragma pack(push, 1)

	enum class SessionType: ui08_t {
		CDDA_OR_CDROM = 0x00,
		CDI = 0x10,
		CDXA_OR_DDCD = 0x20
	};

	static_assert(sizeof(SessionType) == 1);

	#pragma pack(pop)
}
