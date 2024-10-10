#pragma once

#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include "shared.h"

namespace overdrive {
namespace arguments {
	using namespace shared;

	class Parser {
		public:

		std::string key;
		std::string description;
		std::regex regex;
		std::string format;
		bool_t positional;
		std::optional<std::string> default_value;
		bool_t required;
		std::function<void(const std::vector<std::string>& matches)> parser;

		auto parse_named(
			const std::string& key,
			const std::string& value
		) const -> bool_t;

		auto parse_positional(
			size_t& positional_counter,
			size_t& positional_index,
			const std::string& argument
		) const -> bool_t;

		protected:
	};

	auto parse_options_using_parsers(
		const std::vector<std::string>& arguments,
		const std::vector<Parser>& parsers
	) -> void;
}
}
