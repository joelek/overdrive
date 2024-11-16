#include "logger.h"

#include "exceptions.h"

namespace overdrive {
namespace logger {
	Logger::Logger(
		const std::string& path
	) {
		auto handle = pointer<FILE>(nullptr);
		if (path == "stdout") {
			handle = stdout;
		} else if (path == "stderr") {
			handle = stderr;
		} else {
			handle = std::fopen(path.c_str(), "w");
		}
		if (handle == nullptr) {
			OVERDRIVE_THROW(exceptions::IOOpenException(path));
		}
		this->handle = handle;
	}
}
}
