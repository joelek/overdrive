#pragma once

#include "type.h"

namespace overdrive {
namespace sense {
	using namespace type;

	#pragma pack(push, 1)

	enum class SenseKey: ui08_t {
		NO_SENSE = 0x0,
		RECOVERED_ERROR = 0x1,
		NOT_READY = 0x2,
		MEDIUM_ERROR = 0x3,
		HARDWARE_ERROR = 0x4,
		ILLEGAL_REQUEST = 0x5,
		UNIT_ATTENTION = 0x6,
		DATA_PROTECT = 0x7,
		BLANK_CHECK = 0x8,
		VENDOR_SPECIFIC = 0x9,
		COPY_ABORTED = 0xA,
		ABORTED_COMMAND = 0xB,
		OBSOLETE_C = 0xC,
		VOLUME_OVERFLOW = 0xD,
		MISCOMPARE = 0xE,
		RESERVED_F = 0xF
	};

	static_assert(sizeof(SenseKey) == 1);

	enum class ResponseCodes: ui08_t {
		FIXED_CURRENT = 0x70,
		FIXED_DEFERRED = 0x71,
		DESCRIPTOR_CURRENT = 0x72,
		DESCRIPTOR_DEFERRED = 0x73
	};

	static_assert(sizeof(ResponseCodes) == 1);

	struct FixedFormat {
		ResponseCodes response_code: 7;
		ui08_t valid: 1;
		ui08_t : 8;
		SenseKey sense_key: 4;
		ui08_t : 1;
		ui08_t ili: 1;
		ui08_t eom: 1;
		ui08_t filemark: 1;
		ui08_t information[4];
		ui08_t additional_sense_length: 8;
		ui08_t command_specific_information[4];
		ui08_t additional_sense_code: 8;
		ui08_t additional_sense_code_qualifier: 8;
		ui08_t field_replacable_unit_code: 8;
		ui08_t sense_key_specific[3];
	};

	static_assert(sizeof(FixedFormat) == 18);

	#pragma pack(pop)
}
}
