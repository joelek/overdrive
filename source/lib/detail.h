#pragma once

#include <functional>
#include <string>
#include "shared.h"

namespace overdrive {
namespace detail {
	using namespace shared;

	class Detail {
		public:

		std::function<void*(const std::string& drive)> get_handle;
		std::function<byte_t(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)> ioctl;

		protected:
	};

	auto create_detail(
	) -> Detail;
}
}
