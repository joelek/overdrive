#pragma once

#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include "shared.h"

namespace overdrive {
namespace parser {
	using namespace shared;

	class Parser {
		public:

		std::string key;
		std::string description;
		std::regex regex;
		std::string format;
		bool_t positional;
		std::optional<std::string> fallback;
		size_t min_occurences;
		size_t max_occurences;
		std::function<void(const std::vector<std::string>& matches)> parser;

		auto get_matches(
			const std::string& value
		) const -> std::vector<std::string>;

		protected:
	};

	auto parse(
		const std::vector<std::string>& arguments,
		const std::vector<Parser>& parsers
	) -> void;

	auto print(
		const std::vector<Parser>& parsers
	) -> void;

	auto sort(
		std::vector<Parser>& parsers
	) -> void;
}
}
