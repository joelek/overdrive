#include "exceptions.h"

namespace overdrive {
namespace exceptions {
	OverdriveException::OverdriveException(
		const std::string& message
	): std::runtime_error(message) {}

	UnsupportedValueException::UnsupportedValueException(
		const std::string& name
	): OverdriveException(std::format("Expected \"{}\" to not be supported!", name)) {}

	InvalidValueException::InvalidValueException(
		const std::string& name,
		si_t value,
		si_t min,
		si_t max
	): OverdriveException(std::format("Expected value for \"{}\" {} to be at least {} and at most {}!", name, value, min, max)) {}

	MissingValueException::MissingValueException(
		const std::string& name
	): OverdriveException(std::format("Expected value for \"{}\" to be present!", name)) {}

	AutoDetectFailureException::AutoDetectFailureException(
		const std::string& name
	): OverdriveException(std::format("Expected value for \"{}\" to be auto-detected!", name)) {}

	UnreachableCodeReachedException::UnreachableCodeReachedException(
	): OverdriveException(std::format("Expected code to be unreachable!")) {}

	ArgumentException::ArgumentException(
		const std::string& message
	): OverdriveException(message) {}

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

	ExpectedDataTrackException::ExpectedDataTrackException(
		size_t track_number
	): OverdriveException(std::format("Expected track {} to contain data!", track_number)) {}

	ExpectedAudioTrackException::ExpectedAudioTrackException(
		size_t track_number
	): OverdriveException(std::format("Expected track {} to contain audio!", track_number)) {}

	ExpectedOpticalDriveException::ExpectedOpticalDriveException(
	): OverdriveException(std::format("Expected an optical drive!")) {}

	ExpectedOpticalDiscException::ExpectedOpticalDiscException(
	): OverdriveException(std::format("Expected an optical disc!")) {}
}
}
