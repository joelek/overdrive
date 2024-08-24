#include "idiv.h"

auto div_floor(int n, int d)
-> int {
	return n / d - (n % d != 0 && (n ^ d) < 0);
}

auto div_ceil(int n, int d)
-> int {
	return n / d + (n % d != 0 && (n ^ d) > 0);
}

auto div_round(int n, int d)
-> int {
	return (n ^ d) < 0 ? (n - d / 2) / d : (n + d / 2) / d;
}
