#include "drive.h"

#include "../utils/byteswap.h"

namespace scsi {
namespace drive {
	Drive::Drive(
		void* handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& sptd
	) {
		this->handle = handle;
		this->sptd = sptd;
	}

	auto Drive::get_toc(
	) const	-> cdb::ReadTOCResponseNormalTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseNormalTOC();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::NORMAL_TOC;
		cdb.time = 1;
		this->sptd(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_session_info(
	) const	-> cdb::ReadTOCResponseSessionInfo {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseSessionInfo();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::SESSION_INFO;
		cdb.time = 1;
		this->sptd(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_full_toc(
	) const	-> cdb::ReadTOCResponseFullTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseFullTOC();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::FULL_TOC;
		cdb.time = 1;
		this->sptd(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_pma(
	) const	-> cdb::ReadTOCResponsePMA {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponsePMA();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::PMA;
		cdb.time = 1;
		this->sptd(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_atip(
	) const	-> cdb::ReadTOCResponseATIP {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseATIP();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::ATIP;
		cdb.time = 1;
		this->sptd(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}
}
}
