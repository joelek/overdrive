#pragma once

#include <string>
#include "shared.h"

namespace overdrive {
namespace app {
	using namespace shared;

	class Version {
		public:

		size_t major;
		size_t minor;
		size_t patch;

		protected:
	};

	const auto NAME = std::string("Overdrive");
	const auto VERSION = Version({ 1, 0, 0 });
}
}
