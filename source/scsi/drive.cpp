#include "drive.h"

namespace scsi {
namespace drive {
	Drive::Drive(
		void* handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& sptd
	) {
		this->handle = handle;
		this->sptd = sptd;
	}

	auto Drive::get_cdrom_toc(
	) const	-> cdb::ReadTOCResponseNormalTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseNormalTOC();
		cdb.allocation_length_be = sizeof(data);
		cdb.format = cdb::ReadTOCFormat::NORMAL_TOC;
		cdb.track_or_session_number = 1;
		this->sptd(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}
}
}
