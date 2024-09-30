#include "idiv.h"

namespace overdrive {
namespace idiv {
	auto ceil(
		si_t num,
		si_t den
	) -> si_t {
		return num / den + (num % den != 0 && (num ^ den) > 0);
	}

	auto floor(
		si_t num,
		si_t den
	) -> si_t {
		return num / den - (num % den != 0 && (num ^ den) < 0);
	}

	auto round(
		si_t num,
		si_t den
	) -> si_t {
		return (num ^ den) < 0 ? (num - den / 2) / den : (num + den / 2) / den;
	}
}
}
