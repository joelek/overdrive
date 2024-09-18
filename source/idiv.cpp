#include "idiv.h"

namespace idiv {
	auto ceil(
		int n,
		int d
	) -> int {
		return n / d + (n % d != 0 && (n ^ d) > 0);
	}

	auto floor(
		int n,
		int d
	) -> int {
		return n / d - (n % d != 0 && (n ^ d) < 0);
	}

	auto round(
		int n,
		int d
	) -> int {
		return (n ^ d) < 0 ? (n - d / 2) / d : (n + d / 2) / d;
	}
}
