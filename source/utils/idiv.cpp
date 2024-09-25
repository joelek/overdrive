#include "idiv.h"

namespace idiv {
	auto ceil(
		si_t n,
		si_t d
	) -> si_t {
		return n / d + (n % d != 0 && (n ^ d) > 0);
	}

	auto floor(
		si_t n,
		si_t d
	) -> si_t {
		return n / d - (n % d != 0 && (n ^ d) < 0);
	}

	auto round(
		si_t n,
		si_t d
	) -> si_t {
		return (n ^ d) < 0 ? (n - d / 2) / d : (n + d / 2) / d;
	}
}
