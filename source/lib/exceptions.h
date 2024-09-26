#pragma once

#include <stdexcept>
#include "../type.h"

namespace overdrive {
namespace exceptions {
	using namespace type;

	class InvalidValueException: public std::runtime_error {
		public:

		InvalidValueException(
			si_t value,
			si_t min,
			si_t max
		);

		protected:
	};
}
}
