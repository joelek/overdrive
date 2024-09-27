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

		auto get_toc(
		) const -> cdb::ReadTOCResponseNormalTOC;

		auto get_session_info(
		) const -> cdb::ReadTOCResponseSessionInfo;

		auto get_full_toc(
		) const -> cdb::ReadTOCResponseFullTOC;

		auto get_pma(
		) const -> cdb::ReadTOCResponsePMA;

		auto get_atip(
		) const -> cdb::ReadTOCResponseATIP;

		protected:

		void* handle;
		std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)> sptd;
	};
}
}
