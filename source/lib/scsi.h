#pragma once

#include "shared.h"

namespace overdrive {
namespace scsi {
	using namespace shared;

	#pragma pack(push, 1)

	enum class StatusCode: ui08_t {
		GOOD = 0x00,
		CHECK_CONDITION = 0x02,
		CONDITION_MET = 0x04,
		BUSY = 0x08,
		RESERVATION_CONFLICT = 0x18,
		TASK_SET_FULL = 0x28,
		ACA_ACTIVE = 0x30,
		TASK_ABORTED = 0x40
	};

	static_assert(sizeof(StatusCode) == 1);

	#pragma pack(pop)
}
}
