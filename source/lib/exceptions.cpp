#include "exceptions.h"

#include <format>

namespace overdrive {
namespace exceptions {
	InvalidValueException::InvalidValueException(
		si_t value,
		si_t min,
		si_t max
	): std::runtime_error(std::format("Expected value {} to be at least {} and at most {}!", value, min, max)) {}

	MissingValueException::MissingValueException(
	): std::runtime_error(std::format("Expected value to be present!")) {}

	AutoDetectFailureException::AutoDetectFailureException(
	): std::runtime_error(std::format("Expected value to be auto-detected!")) {}

	UnreachableCodeReachedException::UnreachableCodeReachedException(
	): std::runtime_error(std::format("Expected code to be unreachable!")) {}
}
}
