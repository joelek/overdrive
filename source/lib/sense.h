#pragma once

#include "shared.h"

namespace overdrive {
namespace sense {
	using namespace shared;

	namespace SenseKey {
		using type = ui08_t;

		const auto NO_SENSE = type(0x0);
		const auto RECOVERED_ERROR = type(0x1);
		const auto NOT_READY = type(0x2);
		const auto MEDIUM_ERROR = type(0x3);
		const auto HARDWARE_ERROR = type(0x4);
		const auto ILLEGAL_REQUEST = type(0x5);
		const auto UNIT_ATTENTION = type(0x6);
		const auto DATA_PROTECT = type(0x7);
		const auto BLANK_CHECK = type(0x8);
		const auto VENDOR_SPECIFIC = type(0x9);
		const auto COPY_ABORTED = type(0xA);
		const auto ABORTED_COMMAND = type(0xB);
		const auto OBSOLETE_C = type(0xC);
		const auto VOLUME_OVERFLOW = type(0xD);
		const auto MISCOMPARE = type(0xE);
		const auto RESERVED_F = type(0xF);
	}

	namespace ResponseCodes {
		using type = ui08_t;

		const auto FIXED_CURRENT = type(0x70);
		const auto FIXED_DEFERRED = type(0x71);
		const auto DESCRIPTOR_CURRENT = type(0x72);
		const auto DESCRIPTOR_DEFERRED = type(0x73);
	}

	#pragma pack(push, 1)

	struct FixedFormat {
		ResponseCodes::type response_code: 7;
		ui08_t valid: 1;
		ui08_t : 8;
		SenseKey::type sense_key: 4;
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
