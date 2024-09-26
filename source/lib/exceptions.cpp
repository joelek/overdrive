#include "exceptions.h"

#include <format>
#include "../discs/cd.h"

namespace overdrive {
namespace exceptions {
	BadSectorException::BadSectorException(
		ui_t sector
	): std::runtime_error(std::format("Expected sector {} to be at most {}!", sector, discs::cd::MAX_SECTOR)) {}

	InvalidValueException::InvalidValueException(
		si_t value,
		si_t min,
		si_t max
	): std::runtime_error(std::format("Expected value {} to be at least {} and at most {}!", value, min, max)) {}
}
}
