#pragma once

#include <functional>
#include <string>
#include "cd.h"
#include "cdb.h"
#include "type.h"

namespace overdrive {
namespace drive {
	using namespace type;

	#pragma pack(push, 1)

	#pragma pack(pop)

	enum class TrackType {
		AUDIO_2_CHANNELS,
		AUDIO_4_CHANNELS,
		DATA_MODE0,
		DATA_MODE1,
		DATA_MODE2,
		DATA_MODE2_FORM1,
		DATA_MODE2_FORM2
	};

	class DriveInfo {
		public:

		std::string vendor;
		std::string product;
		size_t sector_data_offset;
		size_t subchannels_data_offset;
		size_t c2_data_offset;
		size_t buffer_size;
		bool_t supports_accurate_stream;
		bool_t supports_c2_error_reporting;

		protected:
	};

	class Drive {
		public:

		Drive(
			void* handle,
			size_t sector_data_offset,
			size_t subchannels_data_offset,
			size_t c2_data_offset,
			const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
		);

		auto detect_subchannel_timing_offset(
		) -> si_t;

		auto get_sector_data_offset(
		) const -> size_t;

		auto get_subchannels_data_offset(
		) const -> size_t;

		auto get_c2_data_offset(
		) const -> size_t;

		auto determine_track_type(
			const cdb::ReadTOCResponseFullTOC& toc,
			ui_t track_index
		) const -> TrackType;

		auto read_toc(
		) const -> cdb::ReadTOCResponseNormalTOC;

		auto read_session_info(
		) const -> cdb::ReadTOCResponseSessionInfo;

		auto read_full_toc(
		) const -> cdb::ReadTOCResponseFullTOC;

		auto read_pma(
		) const -> cdb::ReadTOCResponsePMA;

		auto read_atip(
		) const -> cdb::ReadTOCResponseATIP;

		auto read_error_recovery_mode_page(
		) const -> cdb::ModeSenseReadWriteErrorRecoveryModePageResponse;

		auto write_error_recovery_mode_page(
			cdb::ModeSenseReadWriteErrorRecoveryModePageResponse& data
		) const -> void;

		auto read_capabilites_and_mechanical_status_page(
		) const -> cdb::ModeSenseCapabilitiesAndMechanicalStatusPageResponse;

		auto read_standard_inquiry(
		) const -> cdb::StandardInquiryResponse;

		auto read_sector(
			size_t sector_index,
			pointer<array<cd::SECTOR_LENGTH, byte_t>> sector_data,
			pointer<array<cd::SUBCHANNELS_LENGTH, byte_t>> subchannels_data,
			pointer<array<cd::C2_LENGTH, byte_t>> c2_data
		) const -> void;

		auto read_drive_info(
		) const -> DriveInfo;

		protected:

		void* handle;
		size_t sector_data_offset;
		size_t subchannels_data_offset;
		size_t c2_data_offset;
		std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)> ioctl;
	};

	auto create_drive(
		void* handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) -> Drive;
}
}
