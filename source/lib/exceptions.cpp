#include "exceptions.h"

#include <format>

namespace overdrive {
namespace exceptions {
	UnsupportedValueException::UnsupportedValueException(
		const std::string& name
	): std::runtime_error(std::format("Expected \"{}\" to not be supported!", name)) {}

	InvalidValueException::InvalidValueException(
		const std::string& name,
		si_t value,
		si_t min,
		si_t max
	): std::runtime_error(std::format("Expected value for \"{}\" {} to be at least {} and at most {}!", name, value, min, max)) {}

	MissingValueException::MissingValueException(
		const std::string& name
	): std::runtime_error(std::format("Expected value for \"{}\" to be present!", name)) {}

	AutoDetectFailureException::AutoDetectFailureException(
		const std::string& name
	): std::runtime_error(std::format("Expected value for \"{}\" to be auto-detected!", name)) {}

	UnreachableCodeReachedException::UnreachableCodeReachedException(
	): std::runtime_error(std::format("Expected code to be unreachable!")) {}
}
}
