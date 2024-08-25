#include "idiv.h"

auto idiv_floor(int n, int d)
-> int {
	return n / d - (n % d != 0 && (n ^ d) < 0);
}

auto idiv_ceil(int n, int d)
-> int {
	return n / d + (n % d != 0 && (n ^ d) > 0);
}

auto idiv_round(int n, int d)
-> int {
	return (n ^ d) < 0 ? (n - d / 2) / d : (n + d / 2) / d;
}
