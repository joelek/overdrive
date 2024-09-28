#include "drive.h"

#include <array>
#include <cstring>
#include "../lib/exceptions.h"
#include "../utils/bcd.h"
#include "../utils/byteswap.h"

namespace scsi {
namespace drive {
	Drive::Drive(
		void* handle,
		size_t sector_data_offset,
		size_t subchannels_data_offset,
		size_t c2_data_offset,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) {
		this->handle = handle;
		this->sector_data_offset = sector_data_offset;
		this->subchannels_data_offset = subchannels_data_offset;
		this->c2_data_offset = c2_data_offset;
		this->ioctl = ioctl;
	}

	auto Drive::detect_subchannel_timing_offset(
	) -> si_t {
		auto sector = cdb::ReadCDResponseDataA();
		auto deltas = std::array<si_t, 10>();
		auto deltas_index = size_t(0);
		for (auto sector_index = size_t(0); sector_index < size_t(10); sector_index += 1) {
			this->read_sector(sector_index, nullptr, &sector.subchannels_data, nullptr);
			auto subchannels = discs::cd::deinterleave_subchannel_data(sector.subchannels_data);
			auto& q = *reinterpret_cast<discs::cd::SubchannelQ*>(subchannels.channels[discs::cd::SUBCHANNEL_Q_INDEX]);
			if (q.adr == 1) {
				auto decoded_sector_index = discs::cd::get_sector_from_address({ utils::bcd::decode(q.mode1.absolute_address_bcd.m), utils::bcd::decode(q.mode1.absolute_address_bcd.s), utils::bcd::decode(q.mode1.absolute_address_bcd.f) }) - 150;
				auto delta = static_cast<si_t>(sector_index) - static_cast<si_t>(decoded_sector_index);
				if (delta < -10 || delta > 10) {
					throw overdrive::exceptions::AutoDetectFailureException();
				}
				if (deltas_index > 0) {
					if (deltas[deltas_index - 1] != delta) {
						throw overdrive::exceptions::AutoDetectFailureException();
					}
				}
				deltas[deltas_index] = delta;
				deltas_index += 1;
			}
		}
		if (deltas_index >= 9) {
			return deltas[0];
		}
		throw overdrive::exceptions::AutoDetectFailureException();
	}

	auto Drive::get_subchannels_data_offset(
	) const -> size_t {
		return this->subchannels_data_offset;
	}

	auto Drive::get_c2_data_offset(
	) const -> size_t {
		return this->c2_data_offset;
	}

	auto Drive::get_toc(
	) const	-> cdb::ReadTOCResponseNormalTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseNormalTOC();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::NORMAL_TOC;
		cdb.time = 1;
		this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_session_info(
	) const	-> cdb::ReadTOCResponseSessionInfo {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseSessionInfo();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::SESSION_INFO;
		cdb.time = 1;
		this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_full_toc(
	) const	-> cdb::ReadTOCResponseFullTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseFullTOC();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::FULL_TOC;
		cdb.time = 1;
		this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_pma(
	) const	-> cdb::ReadTOCResponsePMA {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponsePMA();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::PMA;
		cdb.time = 1;
		this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::get_atip(
	) const	-> cdb::ReadTOCResponseATIP {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseATIP();
		cdb.allocation_length_be = utils::byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::ATIP;
		cdb.time = 1;
		this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		return data;
	}

	auto Drive::read_sector(
		size_t sector_index,
		pointer<array<discs::cd::SECTOR_LENGTH, byte_t>> sector_data,
		pointer<array<discs::cd::SUBCHANNELS_LENGTH, byte_t>> subchannels_data,
		pointer<array<discs::cd::C2_LENGTH, byte_t>> c2_data
	) const -> void {
		auto cdb = cdb::ReadCD12();
		cdb.expected_sector_type = cdb::ReadCD12ExpectedSectorType::ANY;
		cdb.lba_be = utils::byteswap::byteswap32(sector_index);
		cdb.transfer_length_be[2] = 1;
		cdb.errors = cdb::ReadCD12Errors::C2_ERROR_BLOCK_DATA;
		cdb.edc_and_ecc = 1;
		cdb.user_data = 1;
		cdb.header_codes = cdb::ReadCD12HeaderCodes::ALL_HEADERS;
		cdb.sync = 1;
		cdb.subchannel_selection_bits = cdb::ReadCD12SubchanelBits::RAW;
		auto buffer = std::array<byte_t, cdb::READ_CD_LENGTH>();
		this->ioctl(handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(buffer.data()), sizeof(buffer), false);
		if (sector_data != nullptr) {
			std::memcpy(*sector_data, buffer.data() + this->sector_data_offset, sizeof(*sector_data));
		}
		if (subchannels_data != nullptr) {
			std::memcpy(*subchannels_data, buffer.data() + this->subchannels_data_offset, sizeof(*subchannels_data));
		}
		if (c2_data != nullptr) {
			std::memcpy(*c2_data, buffer.data() + this->c2_data_offset, sizeof(*c2_data));
		}
	}

	auto create_drive(
		void* handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) -> Drive {
		try {
			auto drive = Drive(handle, 0, offsetof(cdb::ReadCDResponseDataA, subchannels_data), offsetof(cdb::ReadCDResponseDataA, c2_data), ioctl);
			drive.detect_subchannel_timing_offset();
			return drive;
		} catch (...) {}
		try {
			auto drive = Drive(handle, 0, offsetof(cdb::ReadCDResponseDataB, subchannels_data), offsetof(cdb::ReadCDResponseDataB, c2_data), ioctl);
			drive.detect_subchannel_timing_offset();
			return drive;
		} catch (...) {}
		throw overdrive::exceptions::AutoDetectFailureException();
	}
}
}
