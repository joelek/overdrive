#pragma once

#include <stdexcept>
#include "../type.h"

namespace overdrive {
namespace exceptions {
	using namespace type;

	class BadSectorException: public std::runtime_error {
		public:

		BadSectorException(
			ui_t sector
		);

		protected:
	};
}
}
