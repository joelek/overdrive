#include "drive.h"

#include <algorithm>
#include <array>
#include <cstring>
#include "accuraterip.h"
#include "bcd.h"
#include "byteswap.h"
#include "cdrom.h"
#include "cdxa.h"
#include "exceptions.h"
#include "iso9660.h"
#include "scsi.h"

namespace overdrive {
namespace drive {
	Drive::Drive(
		void* handle,
		std::optional<size_t> sector_data_offset,
		std::optional<size_t> subchannels_data_offset,
		std::optional<size_t> c2_data_offset,
		const std::function<byte_t(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) {
		this->handle = handle;
		this->sector_data_offset = sector_data_offset;
		this->subchannels_data_offset = subchannels_data_offset;
		this->c2_data_offset = c2_data_offset;
		this->ioctl = ioctl;
	}

	auto Drive::detect_subchannel_timing_offset(
	) const -> si_t {
		auto data = cdb::ReadCDResponseDataA();
		auto deltas = std::array<si_t, 10>();
		auto deltas_index = size_t(0);
		for (auto sector_index = size_t(0); sector_index < size_t(10); sector_index += 1) {
			this->read_sector(sector_index, nullptr, &data.subchannels_data, nullptr);
			auto subchannels = cd::deinterleave_subchannel_data(data.subchannels_data);
			auto& q = *reinterpret_cast<cd::SubchannelQ*>(subchannels.channels[cd::SUBCHANNEL_Q_INDEX]);
			if (q.adr == 1) {
				auto decoded_sector_index = cd::get_sector_from_address(bcd::decode_address(q.mode1.absolute_address_bcd)) - 150;
				auto delta = static_cast<si_t>(sector_index) - static_cast<si_t>(decoded_sector_index);
				if (delta < -10 || delta > 10) {
					OVERDRIVE_THROW(exceptions::AutoDetectFailureException("subchannel timing offset"));
				}
				if (deltas_index > 0) {
					if (deltas[deltas_index - 1] != delta) {
						OVERDRIVE_THROW(exceptions::AutoDetectFailureException("subchannel timing offset"));
					}
				}
				deltas[deltas_index] = delta;
				deltas_index += 1;
			}
		}
		if (deltas_index >= 9) {
			return deltas[0];
		}
		OVERDRIVE_THROW(exceptions::AutoDetectFailureException("subchannel timing offset"));
	}

	auto Drive::get_sector_data_offset(
	) const -> std::optional<size_t> {
		return this->sector_data_offset;
	}

	auto Drive::get_subchannels_data_offset(
	) const -> std::optional<size_t> {
		return this->subchannels_data_offset;
	}

	auto Drive::get_c2_data_offset(
	) const -> std::optional<size_t> {
		return this->c2_data_offset;
	}

	auto Drive::determine_track_type(
		const cdb::ReadTOCResponseFullTOC& toc,
		ui_t track_index
	) const -> disc::TrackType {
		auto& track = toc.entries[track_index];
		auto category = cd::get_track_category(track.control);
		if (category == cd::TrackCategory::AUDIO_2_CHANNELS) {
			return disc::TrackType::AUDIO_2_CHANNELS;
		} else if (category == cd::TrackCategory::AUDIO_4_CHANNELS) {
			return disc::TrackType::AUDIO_4_CHANNELS;
		} else if (category == cd::TrackCategory::DATA) {
			auto session_type = cdb::get_session_type(toc);
			if (session_type == cdb::SessionType::CDDA_OR_CDROM) {
				auto data = cdb::ReadCDResponseDataA();
				this->read_sector(iso9660::PRIMARY_VOLUME_DESCRIPTOR_SECTOR, &data.sector_data, nullptr, nullptr);
				auto& sector = *reinterpret_cast<cdrom::Sector*>(&data.sector_data);
				if (sector.base.header.mode == 0) {
					return disc::TrackType::DATA_MODE0;
				} else if (sector.base.header.mode == 1) {
					return disc::TrackType::DATA_MODE1;
				} else if (sector.base.header.mode == 2) {
					return disc::TrackType::DATA_MODE2;
				} else {
					OVERDRIVE_THROW(exceptions::InvalidValueException("sector mode", sector.base.header.mode, 0, 2));
				}
			} else if (session_type == cdb::SessionType::CDI) {
				OVERDRIVE_THROW(exceptions::UnsupportedValueException("session type CDI"));
			} else if (session_type == cdb::SessionType::CDXA_OR_DDCD) {
				auto data = cdb::ReadCDResponseDataA();
				this->read_sector(iso9660::PRIMARY_VOLUME_DESCRIPTOR_SECTOR, &data.sector_data, nullptr, nullptr);
				auto& sector = *reinterpret_cast<cdxa::Sector*>(&data.sector_data);
				if (sector.base.header.mode == 2) {
					if (sector.base.header_1.form_2 == 0) {
						return disc::TrackType::DATA_MODE2_FORM1;
					} else {
						return disc::TrackType::DATA_MODE2_FORM2;
					}
				} else {
					OVERDRIVE_THROW(exceptions::InvalidValueException("sector mode", sector.base.header.mode, 2, 2));
				}
			}
		} else if (category == cd::TrackCategory::RESERVED) {
			OVERDRIVE_THROW(exceptions::UnsupportedValueException("track category reserved"));
		}
		OVERDRIVE_THROW(exceptions::UnreachableCodeReachedException());
	}

	auto Drive::read_normal_toc(
	) const	-> cdb::ReadTOCResponseNormalTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseNormalTOC();
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::NORMAL_TOC;
		cdb.time = 1;
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_session_info_toc(
	) const	-> cdb::ReadTOCResponseSessionInfoTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseSessionInfoTOC();
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::SESSION_INFO;
		cdb.time = 1;
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_full_toc(
	) const	-> cdb::ReadTOCResponseFullTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseFullTOC();
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::FULL_TOC;
		cdb.time = 1;
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_pma_toc(
	) const	-> cdb::ReadTOCResponsePMATOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponsePMATOC();
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::PMA;
		cdb.time = 1;
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_atip_toc(
	) const	-> cdb::ReadTOCResponseATIPTOC {
		auto cdb = cdb::ReadTOC10();
		auto data = cdb::ReadTOCResponseATIPTOC();
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		cdb.format = cdb::ReadTOCFormat::ATIP;
		cdb.time = 1;
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_error_recovery_mode_page(
	) const -> cdb::ModeSenseReadWriteErrorRecoveryModePageResponse {
		auto cdb = cdb::ModeSense10();
		auto data = cdb::ModeSenseReadWriteErrorRecoveryModePageResponse();
		cdb.page_code = cdb::SensePage::READ_WRITE_ERROR_RECOVERY_MODE_PAGE;
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::write_error_recovery_mode_page(
		cdb::ModeSenseReadWriteErrorRecoveryModePageResponse& data
	) const -> void {
		auto cdb = cdb::ModeSelect10();
		cdb.page_format = 1;
		cdb.parameter_list_length_be = byteswap::byteswap16(sizeof(data));
		auto status = this->ioctl(handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), true);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
	}

	auto Drive::read_caching_mode_page(
	) const -> cdb::ModeSenseCachingModePageResponse {
		auto cdb = cdb::ModeSense10();
		auto data = cdb::ModeSenseCachingModePageResponse();
		cdb.page_code = cdb::SensePage::CACHING_MODE_PAGE;
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::write_caching_mode_page(
		cdb::ModeSenseCachingModePageResponse& data
	) const -> void {
		auto cdb = cdb::ModeSelect10();
		cdb.page_format = 1;
		cdb.parameter_list_length_be = byteswap::byteswap16(sizeof(data));
		auto status = this->ioctl(handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), true);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
	}

	auto Drive::read_capabilites_and_mechanical_status_page(
	) const -> cdb::ModeSenseCapabilitiesAndMechanicalStatusPageResponse {
		auto cdb = cdb::ModeSense10();
		auto data = cdb::ModeSenseCapabilitiesAndMechanicalStatusPageResponse();
		cdb.page_code = cdb::SensePage::CAPABILITIES_AND_MECHANICAL_STATUS_PAGE;
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_standard_inquiry(
	) const -> cdb::StandardInquiryResponse {
		auto cdb = cdb::Inquiry6();
		auto data = cdb::StandardInquiryResponse();
		cdb.allocation_length_be = byteswap::byteswap16(sizeof(data));
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(&data), sizeof(data), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		return data;
	}

	auto Drive::read_all_pages(
	) const -> std::map<cdb::SensePage, std::vector<byte_t>> {
		auto cdb = cdb::ModeSense10();
		auto data = std::vector<byte_t>(65535);
		cdb.page_code = cdb::SensePage::ALL_PAGES;
		cdb.allocation_length_be = byteswap::byteswap16(65535);
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(data.data()), 65535, false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		auto offset = 0;
		auto& header = *reinterpret_cast<cdb::ModeParameterHeader10*>(data.data() + offset);
		offset += sizeof(cdb::ModeParameterHeader10);
		auto length = sizeof(header.mode_data_length_be) + byteswap::byteswap16(header.mode_data_length_be);
		auto map = std::map<cdb::SensePage, std::vector<byte_t>>();
		while (offset < length) {
			auto header_length = sizeof(cdb::ModePageHeader);
			if (offset + header_length > length) {
				OVERDRIVE_THROW(exceptions::MemoryReadException());
			}
			auto& header = *reinterpret_cast<cdb::ModePageHeader*>(data.data() + offset);
			auto total_page_length = sizeof(cdb::ModePageHeader) + header.page_length;
			if (offset + total_page_length > length) {
				OVERDRIVE_THROW(exceptions::MemoryReadException());
			}
			auto key = cdb::SensePage(header.page_code);
			auto value = std::vector<byte_t>(total_page_length);
			std::memcpy(value.data(), data.data() + offset, total_page_length);
			map[key] = std::move(value);
			offset += total_page_length;
		}
		return map;
	}

	auto Drive::test_unit_ready(
	) const -> bool_t {
		auto cdb = cdb::TestUnitReady6();
		auto status = this->ioctl(this->handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), nullptr, 0, false);
		return status == byte_t(scsi::StatusCode::GOOD);
	}

	auto Drive::read_sector(
		size_t sector_index,
		pointer<array<cd::SECTOR_LENGTH, byte_t>> sector_data,
		pointer<array<cd::SUBCHANNELS_LENGTH, byte_t>> subchannels_data,
		pointer<array<cd::C2_LENGTH, byte_t>> c2_data
	) const -> void {
		auto cdb = cdb::ReadCD12();
		cdb.expected_sector_type = cdb::ReadCD12ExpectedSectorType::ANY;
		cdb.lba_be = byteswap::byteswap32(sector_index);
		cdb.transfer_length_be[2] = 1;
		cdb.errors = cdb::ReadCD12Errors::C2_ERROR_BLOCK_DATA;
		cdb.edc_and_ecc = 1;
		cdb.user_data = 1;
		cdb.header_codes = cdb::ReadCD12HeaderCodes::ALL_HEADERS;
		cdb.sync = 1;
		cdb.subchannel_selection_bits = cdb::ReadCD12SubchanelBits::RAW;
		auto buffer = std::array<byte_t, cdb::READ_CD_LENGTH>();
		auto status = this->ioctl(handle, reinterpret_cast<byte_t*>(&cdb), sizeof(cdb), reinterpret_cast<byte_t*>(buffer.data()), sizeof(buffer), false);
		if (scsi::StatusCode(status) != scsi::StatusCode::GOOD) {
			OVERDRIVE_THROW(exceptions::SCSIException());
		}
		if (sector_data != nullptr) {
			if (!this->sector_data_offset) {
				OVERDRIVE_THROW(exceptions::MissingValueException("sector data offset"));
			}
			std::memcpy(*sector_data, buffer.data() + this->sector_data_offset.value(), sizeof(*sector_data));
		}
		if (subchannels_data != nullptr) {
			if (!this->subchannels_data_offset) {
				OVERDRIVE_THROW(exceptions::MissingValueException("subchannels data offset"));
			}
			std::memcpy(*subchannels_data, buffer.data() + this->subchannels_data_offset.value(), sizeof(*subchannels_data));
		}
		if (c2_data != nullptr) {
			if (!this->c2_data_offset) {
				OVERDRIVE_THROW(exceptions::MissingValueException("c2 data offset"));
			}
			std::memcpy(*c2_data, buffer.data() + this->c2_data_offset.value(), sizeof(*c2_data));
		}
	}

	auto Drive::read_drive_info(
	) const -> disc::DriveInfo {
		auto standard_inquiry = this->read_standard_inquiry();
		if (standard_inquiry.peripheral_device_type != cdb::StandardInquiryPeripheralDeviceType::CD_OR_DVD) {
			OVERDRIVE_THROW(exceptions::ExpectedOpticalDriveException());
		}
		auto vendor = std::string(standard_inquiry.vendor_identification, sizeof(standard_inquiry.vendor_identification));
		auto product = std::string(standard_inquiry.product_identification, sizeof(standard_inquiry.product_identification));
		auto sector_data_offset = this->sector_data_offset;
		auto subchannels_data_offset = this->subchannels_data_offset;
		auto c2_data_offset = this->c2_data_offset;
		auto capabilites_and_mechanical_status_page = this->read_capabilites_and_mechanical_status_page();
		auto buffer_size = size_t(byteswap::byteswap16(capabilites_and_mechanical_status_page.page.buffer_size_supported_be)) * 1024;
		auto supports_accurate_stream = capabilites_and_mechanical_status_page.page.cdda_stream_is_accurate == 1;
		auto supports_c2_error_reporting = capabilites_and_mechanical_status_page.page.c2_pointers_supported == 1;
		auto read_offset_correction = accuraterip::DATABASE().get_read_offset_correction_value(standard_inquiry.vendor_identification, standard_inquiry.product_identification);
		return {
			vendor,
			product,
			sector_data_offset,
			subchannels_data_offset,
			c2_data_offset,
			buffer_size,
			supports_accurate_stream,
			supports_c2_error_reporting,
			read_offset_correction
		};
	}

	auto Drive::read_disc_info(
	) const -> disc::DiscInfo {
		if (!this->test_unit_ready()) {
			OVERDRIVE_THROW(exceptions::ExpectedOpticalDiscException());
		}
		auto disc = disc::DiscInfo();
		auto toc = this->read_full_toc();
		auto toc_count = cdb::validate_full_toc(toc);
		auto lead_out_first_sector_absolute = std::optional<size_t>();
		for (auto toc_index = size_t(0); toc_index < toc_count; toc_index += 1) {
			auto& entry = toc.entries[toc_index];
			if (entry.point < 1 || entry.point > 99) {
				if (entry.point == size_t(cdb::ReadTOCResponseFullTOCPoint::LEAD_OUT_TRACK)) {
					lead_out_first_sector_absolute = cd::get_sector_from_address(entry.paddress);
				}
				continue;
			}
			disc.sessions.resize(std::max<size_t>(entry.session_number, disc.sessions.size()));
			auto& session = disc.sessions.at(entry.session_number - 1);
			session.number = entry.session_number;
			session.type = cdb::get_session_type(toc);
			auto track = disc::TrackInfo();
			track.number = entry.point;
			track.type = this->determine_track_type(toc, toc_index);
			track.first_sector_absolute = cd::get_sector_from_address(entry.paddress);
			track.first_sector_relative = track.first_sector_absolute - 150;
			track.length_sectors = 0;
			session.tracks.push_back(track);
		}
		// Sort in increasing order.
		std::sort(disc.sessions.begin(), disc.sessions.end(), [](const disc::SessionInfo& one, const disc::SessionInfo& two) -> bool_t {
			return one.number < two.number;
		});
		for (auto& session : disc.sessions) {
			// Sort in increasing order.
			std::sort(session.tracks.begin(), session.tracks.end(), [](const disc::TrackInfo& one, const disc::TrackInfo& two) -> bool_t {
				return one.first_sector_absolute < two.first_sector_absolute;
			});
			for (auto track_index = size_t(0); track_index < session.tracks.size(); track_index += 1) {
				auto& track = session.tracks.at(track_index);
				if (track_index + 1 < session.tracks.size()) {
					auto& next_track = session.tracks.at(track_index + 1);
					track.length_sectors = next_track.first_sector_absolute - track.first_sector_absolute;
				} else {
					track.length_sectors = lead_out_first_sector_absolute.value() - track.first_sector_absolute;
				}
				track.last_sector_absolute = track.first_sector_absolute + track.length_sectors;
				track.last_sector_relative = track.first_sector_relative + track.length_sectors;
				session.length_sectors += track.length_sectors;
			}
			disc.length_sectors += session.length_sectors;
		}
		return disc;
	}

	auto Drive::set_read_retry_count(
		size_t max_retry_count
	) const -> void {
		auto error_recovery_mode_page = this->read_error_recovery_mode_page();
		error_recovery_mode_page.page.read_retry_count = max_retry_count;
		this->write_error_recovery_mode_page(error_recovery_mode_page);
	}

	auto create_drive(
		void* handle,
		const std::function<byte_t(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) -> Drive {
		try {
			auto drive = Drive(handle, offsetof(cdb::ReadCDResponseDataA, sector_data), offsetof(cdb::ReadCDResponseDataA, subchannels_data), offsetof(cdb::ReadCDResponseDataA, c2_data), ioctl);
			drive.detect_subchannel_timing_offset();
			return drive;
		} catch (const exceptions::AutoDetectFailureException& e) {}
		try {
			auto drive = Drive(handle, offsetof(cdb::ReadCDResponseDataB, sector_data), offsetof(cdb::ReadCDResponseDataB, subchannels_data), offsetof(cdb::ReadCDResponseDataB, c2_data), ioctl);
			drive.detect_subchannel_timing_offset();
			return drive;
		} catch (const exceptions::AutoDetectFailureException& e) {}
		return Drive(handle, std::optional<size_t>(0), std::optional<size_t>(), std::optional<size_t>(), ioctl);
	}
}
}
