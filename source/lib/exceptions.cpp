#include "exceptions.h"

#include <format>

namespace overdrive {
namespace exceptions {
	OverdriveException::OverdriveException(
		const std::string& file,
		size_t line,
		const std::string& message
	): std::runtime_error(file != "" ? std::format("[{}:{}]: {}", file, line, message) : message) {}

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

	ArgumentException::ArgumentException(
		const std::string& message
	): std::runtime_error(message) {}

	BadArgumentException::BadArgumentException(
		const std::string& name,
		const std::string& format
	): ArgumentException(std::format("Expected argument \"{}\" to have format \"{}\"!", name, format)) {}

	MissingArgumentException::MissingArgumentException(
		const std::string& name
	): ArgumentException(std::format("Expected argument \"{}\" to be specified!", name)) {}

	UnknownArgumentException::UnknownArgumentException(
		const std::string& name
	): ArgumentException(std::format("Expected argument \"{}\" to be known!", name)) {}
}
}
