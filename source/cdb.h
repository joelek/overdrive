#pragma once

#include <cstdint>

namespace cdb {
	#pragma pack(push, 1)

	typedef struct {
		const uint8_t operation_code = 0x43;
		uint8_t : 1;
		uint8_t time: 1;
		uint8_t : 3;
		uint8_t : 3;
		uint8_t format: 4;
		uint8_t : 4;
		uint8_t : 8;
		uint8_t : 8;
		uint8_t : 8;
		uint8_t track_or_session_number: 8;
		uint16_t allocation_length_be: 16;
		uint8_t control: 8;
	} ReadTOC;

	typedef struct {
		const uint8_t operation_code = 0x12;
		uint8_t enable_vital_product_data: 1;
		uint8_t obsolete_a: 1;
		uint8_t reserved_a: 6;
		uint8_t page_code;
		uint16_t allocation_length_be;
		uint8_t control;
	} Inquiry6;

	typedef struct {
		uint8_t peripheral_device_type: 5;
		uint8_t peripheral_qualifier: 3;
		uint8_t : 7;
		uint8_t rmb: 1;
		uint8_t version: 8;
		uint8_t response_data_format: 4;
		uint8_t hisup: 1;
		uint8_t normalca: 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t additional_length: 8;
		uint8_t protect: 1;
		uint8_t : 2;
		uint8_t threepc: 1;
		uint8_t tpgs: 2;
		uint8_t acc: 1;
		uint8_t scss: 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t multip: 1;
		uint8_t vs1: 1;
		uint8_t encserv: 1;
		uint8_t : 1;
		uint8_t vs2: 1;
		uint8_t command_queue: 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 1;
		const char vendor_identification[8] = {};
		const char product_identification[16] = {};
		uint32_t product_revision_level_be;
		const char drive_serial_number[8] = {};
		uint8_t vendor_unique[12];
		uint8_t : 1;
		uint8_t : 1;
		uint8_t : 2;
		uint8_t : 4;
		uint8_t : 8;
		uint16_t vendor_descriptor_be[8];
		uint8_t reserved[22];
	} Inquiry6Response;

	enum class SensePage: uint8_t {
		ReadWriteErrorRecoveryModePage = 0x01,
		CapabilitiesAndMechanicalStatusPage = 0x2A
	};

	enum class ModeSensePageControl: uint8_t {
		CURRENT_VALUES = 0b00,
		CHANGABLE_VALUES = 0b01,
		DEFAULT_VALUES = 0b10,
		SAVED_VALUES = 0b11
	};

	typedef struct {
		const uint8_t operation_code = 0x1A;
		uint8_t reserved_a: 3;
		uint8_t disable_block_descriptors: 1;
		uint8_t reserved_b: 4;
		SensePage page_code: 6;
		ModeSensePageControl page_control: 2;
		uint8_t subpage_code;
		uint8_t allocation_length;
		uint8_t control;
	} ModeSense6;

	typedef struct {
		uint8_t mode_data_length;
		uint8_t reserved_a[2];
		uint8_t block_descriptor_length;
	} ModeParameterHeader6;

	typedef struct {
		const uint8_t operation_code = 0x15;
		uint8_t save_pages: 1;
		uint8_t revert_to_defaults: 1;
		uint8_t reserved_a: 2;
		uint8_t page_format: 1;
		uint8_t reserved_b: 3;
		uint8_t reserved_c[2];
		uint8_t parameter_list_length;
		uint8_t control;
	} ModeSelect6;

	typedef struct {
		const uint8_t operation_code = 0x5A;
		uint8_t reserved_a: 3;
		uint8_t disable_block_descriptors: 1;
		uint8_t long_lba_accepted: 1;
		uint8_t reserved_b: 3;
		SensePage page_code: 6;
		ModeSensePageControl page_control: 2;
		uint8_t subpage_code;
		uint8_t reserved_c[3];
		uint16_t allocation_length_be;
		uint8_t control;
	} ModeSense10;

	typedef struct {
		uint16_t mode_data_length_be;
		uint8_t reserved_a[4];
		uint16_t block_descriptor_length_be;
	} ModeParameterHeader10;

	typedef struct {
		const uint8_t operation_code = 0x55;
		uint8_t save_pages: 1;
		uint8_t reserved_a: 3;
		uint8_t page_format: 1;
		uint8_t reserved_b: 3;
		uint8_t reserved_c[5];
		uint16_t parameter_list_length_be;
		uint8_t control;
	} ModeSelect10;

	typedef struct {
		uint8_t page_code: 6;
		uint8_t subpage_format: 1;
		uint8_t parameters_savable: 1;
		uint8_t page_length = 0x0A;
		uint8_t disable_correction: 1;
		uint8_t data_terminate_on_error: 1;
		uint8_t post_error: 1;
		uint8_t enable_early_recovery: 1;
		uint8_t read_continuous: 1;
		uint8_t transfer_block: 1;
		uint8_t automatic_read_reallocation_enabled: 1;
		uint8_t automatic_write_reallocation_enabled: 1;
		uint8_t read_retry_count;
		uint8_t obsolete_a;
		uint8_t obsolete_b;
		uint8_t obsolete_c;
		uint8_t restricted_for_mmc_6: 2;
		uint8_t reserved_a: 6;
		uint8_t write_retry_count;
		uint8_t reserved_b;
		uint16_t recovery_time_limit_be;
	} ReadWriteErrorRecoveryModePage;

	typedef struct {
		uint8_t page_code: 6;
		uint8_t : 1;
		uint8_t parameters_savable: 1;

		uint8_t page_length: 8;

		uint8_t cdr_read: 1;
		uint8_t cdrw_read: 1;
		uint8_t method_2: 1;
		uint8_t dvd_rom_read: 1;
		uint8_t dvd_r_read: 1;
		uint8_t dvd_ram_read: 1;
		uint8_t : 2;

		uint8_t cdr_write: 1;
		uint8_t cdrw_write: 1;
		uint8_t test_write: 1;
		uint8_t : 1;
		uint8_t dvd_r_write: 1;
		uint8_t dvd_ram_write: 1;
		uint8_t : 2;

		uint8_t audio_play: 1;
		uint8_t composite: 1;
		uint8_t digital_port_1: 1;
		uint8_t digital_port_2: 1;
		uint8_t mode_2_form_1: 1;
		uint8_t mode_2_form_2: 1;
		uint8_t multi_session: 1;
		uint8_t buf: 1;

		uint8_t cdda_commands_supported: 1;
		uint8_t cdda_stream_is_accurate: 1;
		uint8_t subchannels_rw_supported: 1;
		uint8_t subchannels_rw_deinterleaved_and_corrected: 1;
		uint8_t c2_pointers_supported: 1;
		uint8_t isrc: 1;
		uint8_t upc: 1;
		uint8_t read_bar_code: 1;

		uint8_t lock: 1;
		uint8_t lock_state: 1;
		uint8_t prevent_jumper: 1;
		uint8_t eject: 1;
		uint8_t : 1;
		uint8_t loading_mechanism_type: 3;

		uint8_t separate_volume_levels: 1;
		uint8_t separate_channel_mute: 1;
		uint8_t changer_supports_disc_present: 1;
		uint8_t sw_slot_selection: 1;
		uint8_t side_change_capable: 1;
		uint8_t rw_in_lead_in: 1;
		uint8_t : 2;

		uint8_t : 8;
		uint8_t : 8;
		uint16_t number_of_volume_levels_supported_be: 16;
		uint16_t buffer_size_supported_be: 16;
		uint8_t : 8;
		uint8_t : 8;
		uint8_t : 8;

		uint8_t : 1;
		uint8_t bckf: 1;
		uint8_t rck: 1;
		uint8_t lsfbf: 1;
		uint8_t length: 2;
		uint8_t : 2;

		uint8_t : 8;
		uint8_t : 8;
		uint8_t : 8;
		uint8_t : 8;
		uint16_t copy_management_revision_supported_be: 16;
		uint8_t : 8;
		uint8_t : 8;
		uint8_t : 8;

		uint8_t rotation_control_selected: 2;
		uint8_t : 6;

		uint16_t current_write_speed_selected_be: 16;
		uint16_t number_of_logical_write_speed_performance_descriptor_tables_be: 16;
	} CapabilitiesAndMechanicalStatusPage;

	enum class ReadCD12ExpectedSectorType: uint8_t {
		ANY = 0b000,
		CD_DA = 0b001,
		CD_ROM_MODE_1 = 0b010,
		CD_ROM_MODE_2 = 0b011,
		CD_XA_MODE_2_FORM_1 = 0b100,
		CD_XA_MODE_2_FORM_2 = 0b101,
		RESERVED_6  = 0b110,
		RESERVED_7 = 0b111
	};

	enum class ReadCD12HeaderCodes: uint8_t {
		NONE = 0b00,
		HEADER_ONLY = 0b01,
		SUBHEADER_ONLY  = 0b10,
		ALL_HEADERS = 0b11
	};

	enum class ReadCD12Errors: uint8_t {
		NONE = 0b00,
		C2_ERROR_BLOCK_DATA = 0b01,
		C2_AND_BLOCK_ERROR_BITS  = 0b10,
		RESERVED_3 = 0b11
	};

	enum class ReadCD12SubchanelBits: uint8_t {
		NONE = 0b000,
		RAW = 0b001,
		Q  = 0b010,
		RESERVED_3 = 0b011,
		P_TO_W = 0b100,
		RESERVED_5 = 0b101,
		RESERVED_6 = 0b110,
		RESERVED_7 = 0b111
	};

	typedef struct {
		const uint8_t operation_code = 0xBE;
		uint8_t real_adr: 1;
		uint8_t: 1;
		ReadCD12ExpectedSectorType expected_sector_type: 3;
		uint8_t: 3;
		uint32_t lba_be;
		uint8_t transfer_length_be[3];
		uint8_t: 1;
		ReadCD12Errors errors: 2;
		uint8_t edc_and_ecc: 1;
		uint8_t user_data: 1;
		ReadCD12HeaderCodes header_codes: 2;
		uint8_t sync: 1;
		ReadCD12SubchanelBits subchannel_selection_bits: 3;
		uint8_t: 5;
		uint8_t control;
	} ReadCD12;

	#pragma pack(pop)
}
