#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <optional>
#include "shared.h"

namespace overdrive {
namespace exceptions {
	using namespace shared;

	template <typename A>
	class DebugException: public A {
		public:

		DebugException(
			const ch08_t* file,
			size_t line,
			A&& wrapped
		): A(wrapped), message(std::format("[{}:{}]: {}", file, line, A::what())) {}

		virtual auto what(
		) const noexcept -> const ch08_t* {
			return this->message.c_str();
		}

		protected:

		std::string message;
	};

	class OverdriveException: public std::runtime_error {
		public:

		OverdriveException(
			const std::string& message
		);

		protected:
	};

	class ExpectedAccurateStreamSupportException: public OverdriveException {
		public:

		ExpectedAccurateStreamSupportException(
		);

		protected:
	};

	class ExpectedC2ErrorReportingSupportException: public OverdriveException {
		public:

		ExpectedC2ErrorReportingSupportException(
		);

		protected:
	};

	class UnsupportedValueException: public OverdriveException {
		public:

		UnsupportedValueException(
			const std::string& name
		);

		protected:
	};

	class InvalidValueException: public OverdriveException {
		public:

		InvalidValueException(
			const std::string& name,
			si_t value,
			std::optional<si_t> min,
			std::optional<si_t> max
		);

		protected:
	};

	class MissingValueException: public OverdriveException {
		public:

		MissingValueException(
			const std::string& name
		);

		protected:
	};

	class AutoDetectFailureException: public OverdriveException {
		public:

		AutoDetectFailureException(
			const std::string& name
		);

		protected:
	};

	class UnreachableCodeReachedException: public OverdriveException {
		public:

		UnreachableCodeReachedException();

		protected:
	};

	class ArgumentException: public OverdriveException {
		public:

		ArgumentException(
			const std::string& message
		);

		protected:
	};

	class BadArgumentFormatException: public ArgumentException {
		public:

		BadArgumentFormatException(
			const std::string& name,
			const std::string& format
		);

		protected:
	};

	class BadArgumentOccurencesException: public ArgumentException {
		public:

		BadArgumentOccurencesException(
			const std::string& name,
			size_t min_occurences,
			size_t max_occurences
		);

		protected:
	};

	class UnrecognizedArgumentException: public ArgumentException {
		public:

		UnrecognizedArgumentException(
			const std::string& name
		);

		protected:
	};

	class ExpectedDataTrackException: public OverdriveException {
		public:

		ExpectedDataTrackException(
			size_t track_number
		);

		protected:
	};

	class ExpectedAudioTrackException: public OverdriveException {
		public:

		ExpectedAudioTrackException(
			size_t track_number
		);

		protected:
	};

	class ExpectedOpticalDriveException: public OverdriveException {
		public:

		ExpectedOpticalDriveException(
		);

		protected:
	};

	class ExpectedOpticalDiscException: public OverdriveException {
		public:

		ExpectedOpticalDiscException(
		);

		protected:
	};

	class SCSIException: public OverdriveException {
		public:

		SCSIException(
			const std::string& message
		);

		protected:
	};

	class InvalidSCSIStatusException: public SCSIException {
		public:

		InvalidSCSIStatusException();

		protected:
	};

	class InvalidSCSIModePageException: public SCSIException {
		public:

		InvalidSCSIModePageException();

		protected:
	};

	class InvalidSCSIModePageSizeException: public SCSIException {
		public:

		InvalidSCSIModePageSizeException(
			const std::string& page,
			size_t page_size,
			size_t device_page_size
		);

		protected:
	};

	class InvalidSCSIModePageWriteException: public SCSIException {
		public:

		InvalidSCSIModePageWriteException();

		protected:
	};

	class IOException: public OverdriveException {
		public:

		IOException(
			const std::string& message
		);

		protected:
	};

	class IOOpenException: public IOException {
		public:

		IOOpenException(
			const std::string& path
		);

		protected:
	};

	class IOWriteException: public IOException {
		public:

		IOWriteException(
			const std::string& path
		);

		protected:
	};

	class MemoryException: public OverdriveException {
		public:

		MemoryException(
			const std::string& message
		);

		protected:
	};

	class MemoryReadException: public MemoryException {
		public:

		MemoryReadException();

		protected:
	};

	class TaskException: public OverdriveException {
		public:

		TaskException(
			const std::string& message
		);

		protected:
	};

	class MissingTaskException: public TaskException {
		public:

		MissingTaskException(
		);

		protected:
	};

	class UnrecognizedTaskException: public TaskException {
		public:

		UnrecognizedTaskException(
			const std::string& key
		);

		protected:
	};

	class WindowsException: public OverdriveException {
		public:

		WindowsException(
			size_t code
		);

		protected:
	};

	class CompressionException: public OverdriveException {
		public:

		CompressionException(
			const std::string& message
		);

		protected:
	};

	class CompressedSizeExceededUncompressedSizeException: public CompressionException {
		public:

		CompressedSizeExceededUncompressedSizeException(
			size_t compressed_byte_count,
			size_t uncompressed_byte_count
		);

		protected:
	};

	class CompressionValidationError: public CompressionException {
		public:

		CompressionValidationError(
		);

		protected:
	};

	class BitWriterSizeExceededError: public OverdriveException {
		public:

		BitWriterSizeExceededError(
			size_t max_size
		);

		protected:
	};
}
}

#ifdef DEBUG
	#define OVERDRIVE_THROW(exception) throw overdrive::exceptions::DebugException(__FILE__, __LINE__, exception)
#else
	#define OVERDRIVE_THROW(exception) throw exception
#endif
