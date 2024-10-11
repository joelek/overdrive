#pragma once

#include "cd.h"
#include "shared.h"

namespace overdrive {
namespace cdb {
	using namespace shared;

	const auto READ_CD_LENGTH = size_t(cd::SECTOR_LENGTH + cd::SUBCHANNELS_LENGTH + cd::C2_LENGTH);
	const auto LEAD_OUT_TRACK_NUMBER = size_t(0xAA);

	#pragma pack(push, 1)

	enum class SessionType: ui08_t {
		CDDA_OR_CDROM = 0x00,
		CDI = 0x10,
		CDXA_OR_DDCD = 0x20
	};

	static_assert(sizeof(SessionType) == 1);

	enum class ReadTOCFormat: ui08_t {
		NORMAL_TOC = 0b0000,
		SESSION_INFO = 0b0001,
		FULL_TOC = 0b0010,
		PMA = 0b0011,
		ATIP = 0b0100,
		CD_TEXT = 0b0101
	};

	static_assert(sizeof(ReadTOCFormat) == 1);

	struct TestUnitReady6 {
		ui08_t operation_code: 8 = 0x00;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t control: 8;
	};

	static_assert(sizeof(TestUnitReady6) == 6);

	struct ReadTOC10 {
		ui08_t operation_code = 0x43;
		ui08_t : 1;
		ui08_t time: 1;
		ui08_t : 3;
		ui08_t : 3;
		ReadTOCFormat format: 4;
		ui08_t : 4;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t track_or_session_number: 8;
		ui16_t allocation_length_be: 16;
		ui08_t control: 8;
	};

	static_assert(sizeof(ReadTOC10) == 10);

	struct ReadTOCResponseParameterList {
		ui16_t data_length_be;
		ui08_t first_track_or_session_number: 8;
		ui08_t last_track_or_session_number: 8;
	};

	static_assert(sizeof(ReadTOCResponseParameterList) == 4);

	struct ReadTOCResponseNormalTOCEntry {
		ui08_t : 8;
		ui08_t control: 4;
		ui08_t adr: 4;
		ui08_t track_number: 8;
		ui08_t : 8;
		ui08_t : 8;
		cd::SectorAddress track_start_address;
	};

	static_assert(sizeof(ReadTOCResponseNormalTOCEntry) == 8);

	struct ReadTOCResponseNormalTOC {
		ReadTOCResponseParameterList header;
		ReadTOCResponseNormalTOCEntry entries[100];
	};

	static_assert(sizeof(ReadTOCResponseNormalTOC) == 804);

	struct ReadTOCResponseSessionInfoTOCEntry {
		ui08_t : 8;
		ui08_t control: 4;
		ui08_t adr: 4;
		ui08_t first_track_number_in_last_complete_session: 8;
		ui08_t : 8;
		ui08_t : 8;
		cd::SectorAddress track_start_address;
	};

	static_assert(sizeof(ReadTOCResponseSessionInfoTOCEntry) == 8);

	struct ReadTOCResponseSessionInfoTOC {
		ReadTOCResponseParameterList header;
		ReadTOCResponseSessionInfoTOCEntry entries[100];
	};

	static_assert(sizeof(ReadTOCResponseSessionInfoTOC) == 804);

	struct ReadTOCResponseFullTOCEntry {
		ui08_t session_number: 8;
		ui08_t control: 4;
		ui08_t adr: 4;
		ui08_t tno: 8;
		ui08_t point: 8;
		cd::SectorAddress address;
		ui08_t phour: 4;
		ui08_t hour: 4;
		cd::SectorAddress paddress;
	};

	static_assert(sizeof(ReadTOCResponseFullTOCEntry) == 11);

	enum class ReadTOCResponseFullTOCPoint: ui08_t {
		FIRST_TRACK = 0xA0,
		LAST_TRACK = 0xA1,
		LEAD_OUT_TRACK = 0xA2,
		B0 = 0xB0,
		B1 = 0xB1,
		B2 = 0xB2,
		B3 = 0xB3,
		B4 = 0xB4,
		C0 = 0xC0,
		C1 = 0xC1
	};

	static_assert(sizeof(ReadTOCResponseFullTOCPoint) == 1);

	struct ReadTOCResponseFullTOC {
		ReadTOCResponseParameterList header;
		ReadTOCResponseFullTOCEntry entries[100];
	};

	static_assert(sizeof(ReadTOCResponseFullTOC) == 1104);

	struct ReadTOCResponsePMATOCEntry {
		ui08_t : 8;
		ui08_t control: 4;
		ui08_t adr: 4;
		ui08_t tno: 8;
		ui08_t point: 8;
		ui08_t min: 8;
		ui08_t sec: 8;
		ui08_t frame: 8;
		ui08_t phour: 4;
		ui08_t hour: 4;
		ui08_t pmin: 8;
		ui08_t psec: 8;
		ui08_t pframe: 8;
	};

	static_assert(sizeof(ReadTOCResponsePMATOCEntry) == 11);

	struct ReadTOCResponsePMATOC {
		ReadTOCResponseParameterList header;
		ReadTOCResponsePMATOCEntry entries[100];
	};

	static_assert(sizeof(ReadTOCResponsePMATOC) == 1104);

	struct ReadTOCResponseATIPTOCEntry {
		ui08_t reference_speed: 3;
		ui08_t ddcd: 1;
		ui08_t indicative_target_writing_power: 4;
		ui08_t : 6;
		ui08_t uru: 1;
		ui08_t zero: 1;
		ui08_t a3_valid: 1;
		ui08_t a2_valid: 1;
		ui08_t a1_valid: 1;
		ui08_t disc_sub_type: 3;
		ui08_t disc_type: 1;
		ui08_t : 1;
		ui08_t : 8;
		cd::SectorAddress astart_time_of_lead_in;
		ui08_t : 8;
		cd::SectorAddress last_possible_start_time_of_lead_out;
		ui08_t : 8;
		ui08_t a1_values[3];
		ui08_t : 8;
		ui08_t a2_values[3];
		ui08_t : 8;
		ui08_t a3_values[3];
		ui08_t : 8;
		ui08_t s4_values[3];
		ui08_t : 8;
	};

	static_assert(sizeof(ReadTOCResponseATIPTOCEntry) == 28);

	struct ReadTOCResponseATIPTOC {
		ReadTOCResponseParameterList header;
		ReadTOCResponseATIPTOCEntry entries[100];
	};

	static_assert(sizeof(ReadTOCResponseATIPTOC) == 2804);

	struct Inquiry6 {
		ui08_t operation_code = 0x12;
		ui08_t enable_vital_product_data: 1;
		ui08_t : 1;
		ui08_t : 6;
		ui08_t page_code;
		ui16_t allocation_length_be;
		ui08_t control;
	};

	static_assert(sizeof(Inquiry6) == 6);

	enum class StandardInquiryPeripheralDeviceType: ui08_t {
		CD_OR_DVD = 0x05
	};

	static_assert(sizeof(StandardInquiryPeripheralDeviceType) == 1);

	struct StandardInquiryResponse {
		StandardInquiryPeripheralDeviceType peripheral_device_type: 5;
		ui08_t peripheral_qualifier: 3;
		ui08_t : 7;
		ui08_t removable_media: 1;
		ui08_t version: 8;
		ui08_t response_data_format: 4;
		ui08_t hisup: 1;
		ui08_t normalca: 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t additional_length: 8;
		ui08_t protect: 1;
		ui08_t : 2;
		ui08_t threepc: 1;
		ui08_t tpgs: 2;
		ui08_t acc: 1;
		ui08_t scss: 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t multip: 1;
		ui08_t vs1: 1;
		ui08_t encserv: 1;
		ui08_t : 1;
		ui08_t vs2: 1;
		ui08_t command_queue: 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 1;
		ch08_t vendor_identification[8];
		ch08_t product_identification[16];
		ui32_t product_revision_level_be;
		ch08_t drive_serial_number[8];
		ui08_t vendor_unique[12];
		ui08_t : 1;
		ui08_t : 1;
		ui08_t : 2;
		ui08_t : 4;
		ui08_t : 8;
		ui16_t vendor_descriptor_be[8];
		ui08_t reserved[22];
		ch08_t copyright_notice[32]; // Note: Size is unspecified.
	};

	static_assert(sizeof(StandardInquiryResponse) == 128);

	enum class SensePage: ui08_t {
		READ_WRITE_ERROR_RECOVERY_MODE_PAGE = 0x01,
		CACHING_MODE_PAGE = 0x08,
		CAPABILITIES_AND_MECHANICAL_STATUS_PAGE = 0x2A,
		ALL_PAGES = 0x3F
	};

	static_assert(sizeof(SensePage) == 1);

	enum class ModeSensePageControl: ui08_t {
		CURRENT_VALUES = 0b00,
		CHANGABLE_VALUES = 0b01,
		DEFAULT_VALUES = 0b10,
		SAVED_VALUES = 0b11
	};

	static_assert(sizeof(ModeSensePageControl) == 1);

	struct ModeSense6 {
		ui08_t operation_code = 0x1A;
		ui08_t : 3;
		ui08_t disable_block_descriptors: 1;
		ui08_t : 4;
		SensePage page_code: 6;
		ModeSensePageControl page_control: 2;
		ui08_t subpage_code;
		ui08_t allocation_length;
		ui08_t control;
	};

	static_assert(sizeof(ModeSense6) == 6);

	struct ModePageHeader {
		ui08_t page_code: 6;
		ui08_t subpage_format: 1;
		ui08_t parameters_savable: 1;
		ui08_t page_length;
	};

	static_assert(sizeof(ModePageHeader) == 2);

	struct ModeParameterHeader6 {
		ui08_t mode_data_length;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t block_descriptor_length;
	};

	static_assert(sizeof(ModeParameterHeader6) == 4);

	struct ModeSelect6 {
		ui08_t operation_code = 0x15;
		ui08_t save_pages: 1;
		ui08_t revert_to_defaults: 1;
		ui08_t : 2;
		ui08_t page_format: 1;
		ui08_t : 3;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t parameter_list_length;
		ui08_t control;
	};

	static_assert(sizeof(ModeSelect6) == 6);

	struct ModeSense10 {
		ui08_t operation_code = 0x5A;
		ui08_t : 3;
		ui08_t disable_block_descriptors: 1;
		ui08_t long_lba_accepted: 1;
		ui08_t : 3;
		SensePage page_code: 6;
		ModeSensePageControl page_control: 2;
		ui08_t subpage_code;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui16_t allocation_length_be;
		ui08_t control;
	};

	static_assert(sizeof(ModeSense10) == 10);

	struct ModeParameterHeader10 {
		ui16_t mode_data_length_be;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui16_t block_descriptor_length_be;
	};

	static_assert(sizeof(ModeParameterHeader10) == 8);

	struct ModeSelect10 {
		ui08_t operation_code = 0x55;
		ui08_t save_pages: 1;
		ui08_t : 3;
		ui08_t page_format: 1;
		ui08_t : 3;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui16_t parameter_list_length_be;
		ui08_t control;
	};

	static_assert(sizeof(ModeSelect10) == 10);

	struct ReadWriteErrorRecoveryModePage {
		ui08_t page_code: 6;
		ui08_t subpage_format: 1;
		ui08_t parameters_savable: 1;
		ui08_t page_length = 0x0A;
		ui08_t disable_correction: 1;
		ui08_t data_terminate_on_error: 1;
		ui08_t post_error: 1;
		ui08_t enable_early_recovery: 1;
		ui08_t read_continuous: 1;
		ui08_t transfer_block: 1;
		ui08_t automatic_read_reallocation_enabled: 1;
		ui08_t automatic_write_reallocation_enabled: 1;
		ui08_t read_retry_count;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t restricted_for_mmc_6: 2;
		ui08_t : 6;
		ui08_t write_retry_count;
		ui08_t : 8;
		ui16_t recovery_time_limit_be;
	};

	static_assert(sizeof(ReadWriteErrorRecoveryModePage) == 12);

	struct CachingModePage {
		ui08_t page_code: 6;
		ui08_t subpage_format: 1;
		ui08_t parameters_savable: 1;
		ui08_t page_length: 8 = 0x12;
		ui08_t rcd: 1;
		ui08_t mf: 1;
		ui08_t wce: 1;
		ui08_t size: 1;
		ui08_t disc: 1;
		ui08_t cap: 1;
		ui08_t abpf: 1;
		ui08_t ic: 1;
		ui08_t write_retention_policy: 4;
		ui08_t demand_read_retention_policy: 4;
		ui16_t disable_prefetch_transfer_length_be: 16;
		ui16_t minimum_prefetch_be: 16;
		ui16_t maximum_prefetch_be: 16;
		ui16_t maximum_prefetch_ceiling_be: 16;
		ui08_t nv_dis: 1;
		ui08_t sync_prog: 2;
		ui08_t vendor_specific: 2;
		ui08_t dra: 1;
		ui08_t lbcss: 1;
		ui08_t fsw: 1;
		ui08_t number_of_cache_segments: 8;
		ui16_t cache_segment_size_be: 16;
		ui08_t reserved: 8;
		ui08_t obsolete[3];
	};

	static_assert(sizeof(CachingModePage) == 20);

	struct CapabilitiesAndMechanicalStatusPage {
		ui08_t page_code: 6;
		ui08_t : 1;
		ui08_t parameters_savable: 1;
		ui08_t page_length: 8;
		ui08_t cdr_read: 1;
		ui08_t cdrw_read: 1;
		ui08_t method_2: 1;
		ui08_t dvd_rom_read: 1;
		ui08_t dvd_r_read: 1;
		ui08_t dvd_ram_read: 1;
		ui08_t : 2;
		ui08_t cdr_write: 1;
		ui08_t cdrw_write: 1;
		ui08_t test_write: 1;
		ui08_t : 1;
		ui08_t dvd_r_write: 1;
		ui08_t dvd_ram_write: 1;
		ui08_t : 2;
		ui08_t audio_play: 1;
		ui08_t composite: 1;
		ui08_t digital_port_1: 1;
		ui08_t digital_port_2: 1;
		ui08_t mode_2_form_1: 1;
		ui08_t mode_2_form_2: 1;
		ui08_t multi_session: 1;
		ui08_t buf: 1;
		ui08_t cdda_commands_supported: 1;
		ui08_t cdda_stream_is_accurate: 1;
		ui08_t subchannels_rw_supported: 1;
		ui08_t subchannels_rw_deinterleaved_and_corrected: 1;
		ui08_t c2_pointers_supported: 1;
		ui08_t isrc: 1;
		ui08_t upc: 1;
		ui08_t read_bar_code: 1;
		ui08_t lock: 1;
		ui08_t lock_state: 1;
		ui08_t prevent_jumper: 1;
		ui08_t eject: 1;
		ui08_t : 1;
		ui08_t loading_mechanism_type: 3;
		ui08_t separate_volume_levels: 1;
		ui08_t separate_channel_mute: 1;
		ui08_t changer_supports_disc_present: 1;
		ui08_t sw_slot_selection: 1;
		ui08_t side_change_capable: 1;
		ui08_t rw_in_lead_in: 1;
		ui08_t : 2;
		ui08_t : 8;
		ui08_t : 8;
		ui16_t number_of_volume_levels_supported_be: 16;
		ui16_t buffer_size_supported_be: 16;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 1;
		ui08_t bckf: 1;
		ui08_t rck: 1;
		ui08_t lsfbf: 1;
		ui08_t length: 2;
		ui08_t : 2;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui16_t copy_management_revision_supported_be: 16;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t : 8;
		ui08_t rotation_control_selected: 2;
		ui08_t : 6;
		ui16_t current_write_speed_selected_be: 16;
		ui16_t number_of_logical_write_speed_performance_descriptor_tables_be: 16;
	};

	static_assert(sizeof(CapabilitiesAndMechanicalStatusPage) == 32);

	enum class ReadCD12ExpectedSectorType: ui08_t {
		ANY = 0b000,
		CD_DA = 0b001,
		CD_ROM_MODE_1 = 0b010,
		CD_ROM_MODE_2 = 0b011,
		CD_XA_MODE_2_FORM_1 = 0b100,
		CD_XA_MODE_2_FORM_2 = 0b101,
		RESERVED_6  = 0b110,
		RESERVED_7 = 0b111
	};

	static_assert(sizeof(ReadCD12ExpectedSectorType) == 1);

	enum class ReadCD12HeaderCodes: ui08_t {
		NONE = 0b00,
		HEADER_ONLY = 0b01,
		SUBHEADER_ONLY  = 0b10,
		ALL_HEADERS = 0b11
	};

	static_assert(sizeof(ReadCD12HeaderCodes) == 1);

	enum class ReadCD12Errors: ui08_t {
		NONE = 0b00,
		C2_ERROR_BLOCK_DATA = 0b01,
		C2_AND_BLOCK_ERROR_BITS  = 0b10,
		RESERVED_3 = 0b11
	};

	static_assert(sizeof(ReadCD12Errors) == 1);

	enum class ReadCD12SubchanelBits: ui08_t {
		NONE = 0b000,
		RAW = 0b001,
		Q  = 0b010,
		RESERVED_3 = 0b011,
		P_TO_W = 0b100,
		RESERVED_5 = 0b101,
		RESERVED_6 = 0b110,
		RESERVED_7 = 0b111
	};

	static_assert(sizeof(ReadCD12SubchanelBits) == 1);

	struct ReadCD12 {
		ui08_t operation_code = 0xBE;
		ui08_t real_adr: 1;
		ui08_t : 1;
		ReadCD12ExpectedSectorType expected_sector_type: 3;
		ui08_t : 3;
		ui32_t lba_be;
		ui08_t transfer_length_be[3];
		ui08_t : 1;
		ReadCD12Errors errors: 2;
		ui08_t edc_and_ecc: 1;
		ui08_t user_data: 1;
		ReadCD12HeaderCodes header_codes: 2;
		ui08_t sync: 1;
		ReadCD12SubchanelBits subchannel_selection_bits: 3;
		ui08_t : 5;
		ui08_t control;
	};

	static_assert(sizeof(ReadCD12) == 12);

	struct ReadCDResponseDataA {
		byte_t sector_data[cd::SECTOR_LENGTH];
		byte_t c2_data[cd::C2_LENGTH];
		byte_t subchannels_data[cd::SUBCHANNELS_LENGTH];
	};

	static_assert(sizeof(ReadCDResponseDataA) == READ_CD_LENGTH);

	struct ReadCDResponseDataB {
		byte_t sector_data[cd::SECTOR_LENGTH];
		byte_t subchannels_data[cd::SUBCHANNELS_LENGTH];
		byte_t c2_data[cd::C2_LENGTH];
	};

	static_assert(sizeof(ReadCDResponseDataB) == READ_CD_LENGTH);

	struct ModeSenseReadWriteErrorRecoveryModePageResponse {
		ModeParameterHeader10 header;
		ReadWriteErrorRecoveryModePage page;
	};

	static_assert(sizeof(ModeSenseReadWriteErrorRecoveryModePageResponse) == 20);

	struct ModeSenseCachingModePageResponse {
		ModeParameterHeader10 header;
		CachingModePage page;
	};

	static_assert(sizeof(ModeSenseCachingModePageResponse) == 28);

	struct ModeSenseCapabilitiesAndMechanicalStatusPageResponse {
		ModeParameterHeader10 header;
		CapabilitiesAndMechanicalStatusPage page;
	};

	static_assert(sizeof(ModeSenseCapabilitiesAndMechanicalStatusPageResponse) == 40);

	#pragma pack(pop)

	auto get_session_type(
		const ReadTOCResponseFullTOC& toc
	) -> SessionType;

	auto validate_session_info_toc(
		const ReadTOCResponseSessionInfoTOC& toc
	) -> size_t;

	auto validate_normal_toc(
		const ReadTOCResponseNormalTOC& toc
	) -> size_t;

	auto validate_full_toc(
		const ReadTOCResponseFullTOC& toc
	) -> size_t;
}
}
