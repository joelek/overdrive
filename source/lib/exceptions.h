#pragma once

#include <format>
#include <stdexcept>
#include <string>
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
			si_t min,
			si_t max
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

	class BadArgumentException: public ArgumentException {
		public:

		BadArgumentException(
			const std::string& name,
			const std::string& format
		);

		protected:
	};

	class MissingArgumentException: public ArgumentException {
		public:

		MissingArgumentException(
			const std::string& name
		);

		protected:
	};

	class UnknownArgumentException: public ArgumentException {
		public:

		UnknownArgumentException(
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
}
}
