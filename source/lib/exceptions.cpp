#include "exceptions.h"

namespace overdrive {
namespace exceptions {
	OverdriveException::OverdriveException(
		const std::string& message
	): std::runtime_error(message) {}

	ExpectedAccurateStreamSupportException::ExpectedAccurateStreamSupportException(
	): OverdriveException(std::format("Expected drive to support accurate stream!")) {}

	ExpectedC2ErrorReportingSupportException::ExpectedC2ErrorReportingSupportException(
	): OverdriveException(std::format("Expected drive to support C2 error reporting!")) {}

	UnsupportedValueException::UnsupportedValueException(
		const std::string& name
	): OverdriveException(std::format("Expected \"{}\" to not be supported!", name)) {}

	InvalidValueException::InvalidValueException(
		const std::string& name,
		si_t value,
		std::optional<si_t> min,
		std::optional<si_t> max
	): OverdriveException(std::format("Expected value for \"{}\" {} to be at least {} and at most {}!", name, value, min ? std::format("{}", min.value()) : "?", max ? std::format("{}", max.value()) : "?")) {}

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

	BadArgumentFormatException::BadArgumentFormatException(
		const std::string& name,
		const std::string& format
	): ArgumentException(std::format("Expected argument \"{}\" to have format {}!", name, format)) {}

	BadArgumentOccurencesException::BadArgumentOccurencesException(
		const std::string& name,
		size_t min_occurences,
		size_t max_occurences
	): ArgumentException(std::format("Expected argument \"{}\" to be specified between {} and {} times!", name, min_occurences, max_occurences)) {}

	UnrecognizedArgumentException::UnrecognizedArgumentException(
		const std::string& name
	): ArgumentException(std::format("Expected argument \"{}\" to be recognized!", name)) {}

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

	SCSIException::SCSIException(
		const std::string& message
	): OverdriveException(message) {}

	InvalidSCSIStatusException::InvalidSCSIStatusException(
	): SCSIException(std::format("Expected SCSI operation to execute successfully!")) {}

	InvalidSCSIModePageException::InvalidSCSIModePageException(
	): SCSIException(std::format("Expected SCSI mode page to exist!")) {}

	InvalidSCSIModePageSizeException::InvalidSCSIModePageSizeException(
		const std::string& page,
		size_t page_size,
		size_t device_page_size
	): SCSIException(std::format("Expected device page size ({}) for mode page {} to be at least {}!", device_page_size, page, page_size)) {}

	InvalidSCSIModePageWriteException::InvalidSCSIModePageWriteException(
	): SCSIException(std::format("Expected SCSI mode page data to only contain changable values!")) {}

	IOException::IOException(
		const std::string& message
	): OverdriveException(message) {}

	IOOpenException::IOOpenException(
		const std::string& path
	): IOException(std::format("Expected open to succeed for path \"{}\"!", path)) {}

	IOWriteException::IOWriteException(
		const std::string& path
	): IOException(std::format("Expected write to succeed for path \"{}\"!", path)) {}

	MemoryException::MemoryException(
		const std::string& message
	): OverdriveException(message) {}

	MemoryReadException::MemoryReadException(
	): MemoryException(std::format("Expected memory read operation to be within bounds!")) {}

	CommandException::CommandException(
		const std::string& message
	): OverdriveException(message) {}

	MissingCommandException::MissingCommandException(
	): CommandException(std::format("Expected command to be specified!")) {}

	UnrecognizedCommandException::UnrecognizedCommandException(
		const std::string& key
	): CommandException(std::format("Expected command \"{}\" to be recognized!", key)) {}

	WindowsException::WindowsException(
		size_t code
	): OverdriveException(std::format("Expected WINAPI to not return error code {}!", code)) {}

	CompressionException::CompressionException(
		const std::string& message
	): OverdriveException(message) {}

	CompressedSizeExceededUncompressedSizeException::CompressedSizeExceededUncompressedSizeException(
		size_t compressed_byte_count,
		size_t uncompressed_byte_count
	): CompressionException(std::format("Expected compressed size of {} to not exceed the uncompressed size of {}!", compressed_byte_count, uncompressed_byte_count)) {}

	CompressionValidationError::CompressionValidationError(
	): CompressionException(std::format("Expected the decompressed data to be identical to the uncompressed data!")) {}

	BitWriterSizeExceededError::BitWriterSizeExceededError(
		size_t max_size
	): OverdriveException(std::format("Expected BitWriter size not to exceed {} bytes!", max_size)) {}
}
}
