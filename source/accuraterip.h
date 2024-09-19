#pragma once

#include <map>
#include <string>
#include <optional>
#include "type.h"

namespace accuraterip {
	using namespace type;

	class Database {
		public:

		Database(
		);

		auto get_read_offset_correction_value(
			reference<array<8, constant<ch08_t>>> vendor,
			reference<array<16, constant<ch08_t>>> product
		) -> std::optional<si_t>;

		protected:

		std::map<std::string, si_t> read_offset_correction_values;
	};
}
