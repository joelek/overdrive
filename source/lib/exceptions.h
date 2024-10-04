#pragma once

#include <stdexcept>
#include <string>
#include "type.h"

namespace overdrive {
namespace exceptions {
	using namespace type;

	class OverdriveException: public std::runtime_error {
		public:

		OverdriveException(
			const std::string& file,
			size_t line,
			const std::string& message
		);

		protected:
	};

	class UnsupportedValueException: public std::runtime_error {
		public:

		UnsupportedValueException(
			const std::string& name
		);

		protected:
	};

	class InvalidValueException: public std::runtime_error {
		public:

		InvalidValueException(
			const std::string& name,
			si_t value,
			si_t min,
			si_t max
		);

		protected:
	};

	class MissingValueException: public std::runtime_error {
		public:

		MissingValueException(
			const std::string& name
		);

		protected:
	};

	class AutoDetectFailureException: public std::runtime_error {
		public:

		AutoDetectFailureException(
			const std::string& name
		);

		protected:
	};

	class UnreachableCodeReachedException: public std::runtime_error {
		public:

		UnreachableCodeReachedException();

		protected:
	};

	class ArgumentException: public std::runtime_error {
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
}
}
