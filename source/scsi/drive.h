#pragma once

#include <functional>
#include "../type.h"
#include "cdb.h"

namespace scsi {
namespace drive {
	using namespace type;

	#pragma pack(push, 1)

	#pragma pack(pop)

	class Drive {
		public:

		Drive(
			void* handle,
			const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& sptd
		);

		auto get_cdrom_toc(
		) const -> cdb::ReadTOCResponseNormalTOC;

		protected:

		void* handle;
		std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)> sptd;
	};
}
}
