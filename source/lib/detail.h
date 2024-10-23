#pragma once

#include <functional>
#include <string>
#include "scsi.h"
#include "shared.h"

namespace overdrive {
namespace detail {
	using namespace shared;

	using get_handle_t = std::function<void*(const std::string& drive)>;
	using ioctl_t = std::function<scsi::StatusCode::type(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, pointer<array<255, byte_t>> sense, bool_t write_to_device)>;

	class Detail {
		public:

		get_handle_t get_handle;
		ioctl_t ioctl;

		protected:
	};

	auto create_detail(
	) -> Detail;
}
}
