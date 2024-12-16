#include "emulator.h"

#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>
#include "byteswap.h"
#include "cd.h"
#include "cdb.h"
#include "exceptions.h"
#include "path.h"
#include "scsi.h"

namespace overdrive {
namespace emulator {
	namespace internal {
	namespace {
		auto create_capabilities_and_mechanical_status_page(
			cdb::ModeSensePageControl::type page_control
		) -> cdb::CapabilitiesAndMechanicalStatusPage {
			auto page = cdb::CapabilitiesAndMechanicalStatusPage();
			page.page_code = cdb::SensePage::CAPABILITIES_AND_MECHANICAL_STATUS_PAGE;
			page.page_length = sizeof(page) - sizeof(cdb::ModePageHeader);
			if (page_control != cdb::ModeSensePageControl::CHANGABLE_VALUES) {
				page.buffer_size_supported_be = byteswap::byteswap16_on_little_endian_systems(2048);
				page.cdda_stream_is_accurate = 1;
				page.c2_pointers_supported = 1;
			}
			return page;
		}

		auto create_caching_mode_page(
			cdb::ModeSensePageControl::type page_control
		) -> cdb::CachingModePage {
			auto page = cdb::CachingModePage();
			page.page_code = cdb::SensePage::CACHING_MODE_PAGE;
			page.page_length = sizeof(page) - sizeof(cdb::ModePageHeader);
			if (page_control != cdb::ModeSensePageControl::CHANGABLE_VALUES) {}
			return page;
		}

		auto create_read_write_error_recovery_mode_page(
			cdb::ModeSensePageControl::type page_control
		) -> cdb::ReadWriteErrorRecoveryModePage {
			auto page = cdb::ReadWriteErrorRecoveryModePage();
			page.page_code = cdb::SensePage::READ_WRITE_ERROR_RECOVERY_MODE_PAGE;
			page.page_length = sizeof(page) - sizeof(cdb::ModePageHeader);
			if (page_control != cdb::ModeSensePageControl::CHANGABLE_VALUES) {}
			return page;
		}

		auto handle_inquiry_6(
			const ImageAdapter& image_adapter,
			void* handle,
			const cdb::Inquiry6& cdb,
			byte_t* data,
			size_t data_size
		) -> scsi::StatusCode::type {
			(void)image_adapter;
			(void)handle;
			(void)cdb;
			if (data_size < sizeof(cdb::StandardInquiryResponse)) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			auto& response = *reinterpret_cast<cdb::StandardInquiryResponse*>(data);
			response.peripheral_device_type = cdb::StandardInquiryPeripheralDeviceType::CD_OR_DVD;
			std::memcpy(response.vendor_identification, "OD      ", sizeof(response.vendor_identification));
			std::memcpy(response.product_identification, "Image Drive     ", sizeof(response.product_identification));
			return scsi::StatusCode::GOOD;
		}

		auto handle_test_unit_ready_6(
			const ImageAdapter& image_adapter,
			void* handle,
			const cdb::TestUnitReady6& cdb,
			byte_t* data,
			size_t data_size
		) -> scsi::StatusCode::type {
			(void)image_adapter;
			(void)handle;
			(void)cdb;
			(void)data;
			(void)data_size;
			return scsi::StatusCode::GOOD;
		}

		auto handle_mode_sense_10(
			const ImageAdapter& image_adapter,
			void* handle,
			const cdb::ModeSense10& cdb,
			byte_t* data,
			size_t data_size
		) -> scsi::StatusCode::type {
			(void)image_adapter;
			(void)handle;
			if (cdb.page_code == cdb::SensePage::CAPABILITIES_AND_MECHANICAL_STATUS_PAGE) {
				if (data_size < sizeof(cdb::ModeSenseCapabilitiesAndMechanicalStatusPageResponse)) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				auto& response = *reinterpret_cast<cdb::ModeSenseCapabilitiesAndMechanicalStatusPageResponse*>(data);
				response.header.mode_data_length_be = byteswap::byteswap16_on_little_endian_systems(sizeof(response) - sizeof(response.header.mode_data_length_be));
				response.page = create_capabilities_and_mechanical_status_page(cdb.page_control);
				return scsi::StatusCode::GOOD;
			}
			if (cdb.page_code == cdb::SensePage::CACHING_MODE_PAGE) {
				if (data_size < sizeof(cdb::ModeSenseCachingModePageResponse)) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				auto& response = *reinterpret_cast<cdb::ModeSenseCachingModePageResponse*>(data);
				response.header.mode_data_length_be = byteswap::byteswap16_on_little_endian_systems(sizeof(response) - sizeof(response.header.mode_data_length_be));
				response.page = create_caching_mode_page(cdb.page_control);
				return scsi::StatusCode::GOOD;
			}
			if (cdb.page_code == cdb::SensePage::READ_WRITE_ERROR_RECOVERY_MODE_PAGE) {
				if (data_size < sizeof(cdb::ModeSenseReadWriteErrorRecoveryModePageResponse)) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				auto& response = *reinterpret_cast<cdb::ModeSenseReadWriteErrorRecoveryModePageResponse*>(data);
				response.header.mode_data_length_be = byteswap::byteswap16_on_little_endian_systems(sizeof(response) - sizeof(response.header.mode_data_length_be));
				response.page = create_read_write_error_recovery_mode_page(cdb.page_control);
				return scsi::StatusCode::GOOD;
			}
			if (cdb.page_code == cdb::SensePage::ALL_PAGES) {
				auto min_size = sizeof(cdb::ModeParameterHeader10) + sizeof(cdb::CapabilitiesAndMechanicalStatusPage) + sizeof(cdb::ReadWriteErrorRecoveryModePage) + sizeof(cdb::CachingModePage);
				if (data_size < min_size) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				auto offset = 0;
				auto& header = *reinterpret_cast<cdb::ModeParameterHeader10*>(data + offset);
				header.mode_data_length_be = byteswap::byteswap16_on_little_endian_systems(min_size - sizeof(header.mode_data_length_be));
				offset += sizeof(header);
				auto& p1 = *reinterpret_cast<cdb::CapabilitiesAndMechanicalStatusPage*>(data + offset);
				offset += sizeof(p1);
				p1 = create_capabilities_and_mechanical_status_page(cdb.page_control);
				auto& p2 = *reinterpret_cast<cdb::ReadWriteErrorRecoveryModePage*>(data + offset);
				offset += sizeof(p2);
				p2 = create_read_write_error_recovery_mode_page(cdb.page_control);
				auto& p3 = *reinterpret_cast<cdb::CachingModePage*>(data + offset);
				offset += sizeof(p3);
				p3 = create_caching_mode_page(cdb.page_control);
				return scsi::StatusCode::GOOD;
			}
			return scsi::StatusCode::CHECK_CONDITION;
		}

		auto handle_mode_select_10(
			const ImageAdapter& image_adapter,
			void* handle,
			const cdb::ModeSelect10& cdb,
			byte_t* data,
			size_t data_size
		) -> scsi::StatusCode::type {
			(void)image_adapter;
			(void)handle;
			(void)cdb;
			(void)data;
			(void)data_size;
			return scsi::StatusCode::CHECK_CONDITION;
		}

		auto handle_read_toc_10(
			const ImageAdapter& image_adapter,
			void* handle,
			const cdb::ReadTOC10& cdb,
			byte_t* data,
			size_t data_size
		) -> scsi::StatusCode::type {
			if (cdb.format == cdb::ReadTOCFormat::NORMAL_TOC) {
				auto point_table = cdb::ReadTOCResponseFullTOC();
				auto point_count = image_adapter.read_point_table(handle, reinterpret_cast<byte_t*>(&point_table.entries), sizeof(point_table.entries));
				auto track_indices = std::vector<size_t>();
				auto first_track_index = std::optional<size_t>();
				auto last_track_index = std::optional<size_t>();
				auto lead_out_track_index = std::optional<size_t>();
				for (auto point_index = size_t(0); point_index < point_count; point_index += 1) {
					auto& entry = point_table.entries[point_index];
					if (entry.adr == 1 && (entry.point >= cdb::ReadTOCResponseFullTOCPoint::FIRST_TRACK_REFERENCE && entry.point <= cdb::ReadTOCResponseFullTOCPoint::LAST_TRACK_REFERENCE)) {
						track_indices.push_back(point_index);
						continue;
					}
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::FIRST_TRACK_IN_SESSION) {
						first_track_index = point_index;
						continue;
					}
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::LAST_TRACK_IN_SESSION) {
						last_track_index = point_index;
						continue;
					}
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::LEAD_OUT_TRACK_IN_SESSION) {
						lead_out_track_index = point_index;
						continue;
					}
				}
				if (!first_track_index) {
					OVERDRIVE_THROW(exceptions::MissingValueException("first track index"));
				}
				if (!last_track_index) {
					OVERDRIVE_THROW(exceptions::MissingValueException("last track index"));
				}
				if (!lead_out_track_index) {
					OVERDRIVE_THROW(exceptions::MissingValueException("lead out track index"));
				}
				track_indices.push_back(lead_out_track_index.value());
				auto size = sizeof(cdb::ReadTOCResponseNormalTOC::header) + track_indices.size() * sizeof(cdb::ReadTOCResponseNormalTOCEntry);
				if (data_size < size) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				auto& response = *reinterpret_cast<cdb::ReadTOCResponseNormalTOC*>(data);
				response.header.data_length_be = byteswap::byteswap16_on_little_endian_systems(size - sizeof(response.header.data_length_be));
				response.header.first_track_or_session_number = point_table.entries[first_track_index.value()].paddress.m;
				response.header.last_track_or_session_number = point_table.entries[last_track_index.value()].paddress.m;
				for (auto track_index = size_t(0); track_index < track_indices.size(); track_index += 1) {
					auto& entry = point_table.entries[track_indices.at(track_index)];
					auto response_entry = cdb::ReadTOCResponseNormalTOCEntry();
					response_entry.control = entry.control;
					response_entry.adr = entry.adr;
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::LEAD_OUT_TRACK_IN_SESSION) {
						response_entry.track_number = 0xAA;
					} else {
						response_entry.track_number = entry.point;
					}
					response_entry.track_start_address = entry.paddress;
					response.entries[track_index] = response_entry;
				}
				return scsi::StatusCode::GOOD;
			}
			if (cdb.format == cdb::ReadTOCFormat::FULL_TOC) {
				auto point_table = cdb::ReadTOCResponseFullTOC();
				auto point_count = image_adapter.read_point_table(handle, reinterpret_cast<byte_t*>(&point_table.entries), sizeof(point_table.entries));
				auto first_track_index = std::optional<size_t>();
				auto last_track_index = std::optional<size_t>();
				auto lead_out_track_index = std::optional<size_t>();
				for (auto point_index = size_t(0); point_index < point_count; point_index += 1) {
					auto& entry = point_table.entries[point_index];
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::FIRST_TRACK_IN_SESSION) {
						first_track_index = point_index;
						continue;
					}
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::LAST_TRACK_IN_SESSION) {
						last_track_index = point_index;
						continue;
					}
					if (entry.adr == 1 && entry.point == cdb::ReadTOCResponseFullTOCPoint::LEAD_OUT_TRACK_IN_SESSION) {
						lead_out_track_index = point_index;
						continue;
					}
				}
				if (!first_track_index) {
					OVERDRIVE_THROW(exceptions::MissingValueException("first track index"));
				}
				if (!last_track_index) {
					OVERDRIVE_THROW(exceptions::MissingValueException("last track index"));
				}
				if (!lead_out_track_index) {
					OVERDRIVE_THROW(exceptions::MissingValueException("lead out track index"));
				}
				auto size = sizeof(cdb::ReadTOCResponseFullTOC::header) + point_count * sizeof(cdb::ReadTOCResponseFullTOCEntry);
				if (data_size < size) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				auto& response = *reinterpret_cast<cdb::ReadTOCResponseFullTOC*>(data);
				response.header.data_length_be = byteswap::byteswap16_on_little_endian_systems(size - sizeof(response.header.data_length_be));
				response.header.first_track_or_session_number = point_table.entries[first_track_index.value()].session_number;
				response.header.last_track_or_session_number = point_table.entries[last_track_index.value()].session_number;
				for (auto point_index = size_t(0); point_index < point_count; point_index += 1) {
					auto& entry = point_table.entries[point_index];
					response.entries[point_index] = entry;
				}
				return scsi::StatusCode::GOOD;
			}
			return scsi::StatusCode::CHECK_CONDITION;
		}

		auto handle_read_cd_msf_12(
			const ImageAdapter& image_adapter,
			void* handle,
			const cdb::ReadCDMSF12& cdb,
			byte_t* data,
			size_t data_size
		) -> scsi::StatusCode::type {
			if (cdb.edc_and_ecc != 1) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			if (cdb.user_data != 1) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			if (cdb.header_codes != cdb::ReadCD12HeaderCodes::ALL_HEADERS) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			if (cdb.sync != 1) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			if (cdb.subchannel_selection_bits != cdb::ReadCD12SubchanelBits::RAW) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			if (cdb.errors != cdb::ReadCD12Errors::C2_ERROR_BLOCK_DATA) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			auto sector_size = cd::SECTOR_LENGTH + cd::SUBCHANNELS_LENGTH + cd::C2_LENGTH;
			auto start_sector = cd::get_sector_from_address(cdb.start_address);
			auto end_sector_exclusive = cd::get_sector_from_address(cdb.end_address_exclusive);
			if (end_sector_exclusive < start_sector) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			auto size = sector_size * (end_sector_exclusive - start_sector);
			if (data_size < size) {
				return scsi::StatusCode::CHECK_CONDITION;
			}
			auto offset = size_t(0);
			for (auto sector_index = start_sector; sector_index < end_sector_exclusive; sector_index += 1) {
				auto success = image_adapter.read_sector_data(handle, data + offset, sector_size, sector_index);
				if (!success) {
					return scsi::StatusCode::CHECK_CONDITION;
				}
				offset += sector_size;
			}
			return scsi::StatusCode::GOOD;
		}

		auto get_handle(
			const std::string& drive
		) -> void* {
			auto path = std::string(path::create_path(drive));
			auto handle = std::fopen(path.c_str(), "rb");
			if (handle == nullptr) {
				OVERDRIVE_THROW(exceptions::IOOpenException(path));
			}
			return handle;
		}

		auto ioctl(
			void* handle,
			byte_t* cdb,
			size_t cdb_size,
			byte_t* data,
			size_t data_size,
			pointer<array<255, byte_t>> sense,
			bool_t write_to_device,
			const ImageAdapter& image_adapter
		) -> scsi::StatusCode::type {
			(void)sense;
			(void)write_to_device;
			if (cdb_size >= 6 && cdb[0] == 0x12) {
				return internal::handle_inquiry_6(image_adapter, handle, *reinterpret_cast<cdb::Inquiry6*>(cdb), data, data_size);
			}
			if (cdb_size >= 6 && cdb[0] == 0x00) {
				return internal::handle_test_unit_ready_6(image_adapter, handle, *reinterpret_cast<cdb::TestUnitReady6*>(cdb), data, data_size);
			}
			if (cdb_size >= 10 && cdb[0] == 0x5A) {
				return internal::handle_mode_sense_10(image_adapter, handle, *reinterpret_cast<cdb::ModeSense10*>(cdb), data, data_size);
			}
			if (cdb_size >= 10 && cdb[0] == 0x55) {
				return internal::handle_mode_select_10(image_adapter, handle, *reinterpret_cast<cdb::ModeSelect10*>(cdb), data, data_size);
			}
			if (cdb_size >= 10 && cdb[0] == 0x43) {
				return internal::handle_read_toc_10(image_adapter, handle, *reinterpret_cast<cdb::ReadTOC10*>(cdb), data, data_size);
			}
			if (cdb_size >= 12 && cdb[0] == 0xB9) {
				return internal::handle_read_cd_msf_12(image_adapter, handle, *reinterpret_cast<cdb::ReadCDMSF12*>(cdb), data, data_size);
			}
			return scsi::StatusCode::CHECK_CONDITION;
		}
	}
	}

	auto create_detail(
		const ImageAdapter& image_adapter
	) -> detail::Detail {
		auto get_handle = internal::get_handle;
		auto ioctl = [&, image_adapter](void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, pointer<array<255, byte_t>> sense, bool_t write_to_device) -> scsi::StatusCode::type {
			return internal::ioctl(handle, cdb, cdb_size, data, data_size, sense, write_to_device, image_adapter);
		};
		return {
			get_handle,
			ioctl
		};
	}
}
}
