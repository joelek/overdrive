#pragma once

#include <functional>
#include "detail.h"
#include "shared.h"

namespace overdrive {
namespace emulator {
	using namespace shared;

	using read_point_table_t = std::function<size_t(void* handle, byte_t* data, size_t data_size)>;
	using read_sector_data_t = std::function<bool_t(void* handle, byte_t* data, size_t data_size, si_t absolute_sector)>;

	class ImageAdapter {
		public:

		read_point_table_t read_point_table;
		read_sector_data_t read_sector_data;

		protected:
	};

	auto create_detail(
		const ImageAdapter& image_adapter
	) -> detail::Detail;
}
}
