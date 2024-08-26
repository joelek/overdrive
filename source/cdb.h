#pragma once

#include <cstdint>

namespace cdb {
	#pragma pack(push, 1)

	typedef struct {
		const uint8_t operation_code = 0x12;
		uint8_t enable_vital_product_data: 1;
		uint8_t obsolete_a: 1;
		uint8_t reserved_a: 6;
		uint8_t page_code;
		uint16_t allocation_length_be;
		uint8_t control;
	} Inquiry6;

	enum class SensePage: uint8_t {
		ReadWriteErrorRecoveryModePage = 0x01
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
