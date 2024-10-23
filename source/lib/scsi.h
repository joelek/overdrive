#pragma once

#include "shared.h"

namespace overdrive {
namespace scsi {
	using namespace shared;

	namespace StatusCode {
		using type = ui08_t;

		const auto GOOD = type(0x00);
		const auto CHECK_CONDITION = type(0x02);
		const auto CONDITION_MET = type(0x04);
		const auto BUSY = type(0x08);
		const auto RESERVATION_CONFLICT = type(0x18);
		const auto TASK_SET_FULL = type(0x28);
		const auto ACA_ACTIVE = type(0x30);
		const auto TASK_ABORTED = type(0x40);
	}
}
}
