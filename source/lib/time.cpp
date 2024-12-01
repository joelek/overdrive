#include "time.h"

#include <chrono>

namespace overdrive {
namespace time {
	auto get_duration_ms(
		si64_t start_ms = 0
	) -> si64_t {
		auto now_ms = get_time_ms();
		auto duration_ms = now_ms - start_ms;
		return duration_ms;
	}

	auto get_time_ms(
	) -> si64_t {
		auto now = std::chrono::high_resolution_clock::now();
		auto since_epoch = now.time_since_epoch();
		auto now_ms = si64_t(std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch).count());
		return now_ms;
	}
}
}
