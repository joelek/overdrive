#pragma once

#include <stdexcept>
#include <string>
#include "type.h"

namespace overdrive {
namespace exceptions {
	using namespace type;

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
}
}
