#include "exceptions.h"

#include <format>
#include "../discs/cd.h"

namespace overdrive {
namespace exceptions {
	BadSectorException::BadSectorException(
		ui_t sector
	): std::runtime_error(std::format("Expected sector {} to be at most {}!", sector, discs::cd::MAX_SECTOR)) {}
}
}
