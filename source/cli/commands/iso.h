#pragma once

#include <functional>
#include <string>
#include <vector>
#include "../../lib/overdrive.h"

using namespace overdrive;
using namespace type;

namespace commands {
	auto iso(
		const std::vector<std::string>& arguments,
		const std::function<void*(const std::string& drive)>& get_handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) -> void;
}
