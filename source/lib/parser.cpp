#include "parser.h"

#include <algorithm>
#include "exceptions.h"
#include "string.h"
#include "vector.h"

namespace overdrive {
namespace parser {
	auto Parser::get_matches(
		const std::string& value
	) const -> std::vector<std::string> {
		auto matches = std::vector<std::string>();
		auto parts = string::split(value, ",");
		for (auto part_index = size_t(0); part_index < parts.size(); part_index += 1) {
			auto& part = parts.at(part_index);
			auto part_matches = std::vector<std::string>();
			if (string::match(part, part_matches, this->regex)) {
				vector::append(matches, part_matches);
			} else {
				OVERDRIVE_THROW(exceptions::BadArgumentFormatException(this->key, this->format));
			}
		}
		return matches;
	}

	auto parse(
		const std::vector<std::string>& arguments,
		const std::vector<Parser>& parsers
	) -> void {
		auto positional_index = size_t(0);
		auto parser_matches_vector = std::vector<std::vector<std::string>>(parsers.size());
		for (auto argument_index = size_t(0); argument_index < arguments.size(); argument_index += 1) {
			auto& argument = arguments[argument_index];
			auto parsed = false;
			auto key_val_matches = std::vector<std::string>();
			if (string::match(argument, key_val_matches, std::regex("^[-][-]([^=]+)[=]([^=]*)$"))) {
				auto& key = key_val_matches.at(0);
				auto& value = key_val_matches.at(1);
				for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
					auto& parser = parsers.at(parser_index);
					if (parser.key == key) {
						auto matches = parser.get_matches(value);
						auto& parser_matches = parser_matches_vector.at(parser_index);
						vector::append(parser_matches, matches);
						parsed = true;
					}
					for (auto alias_index = size_t(0); alias_index < parser.aliases.size(); alias_index += 1) {
						auto& alias = parser.aliases.at(alias_index);
						if (alias == key) {
							auto matches = parser.get_matches(value);
							auto& parser_matches = parser_matches_vector.at(parser_index);
							vector::append(parser_matches, matches);
							parsed = true;
						}
					}
				}
			} else if (!string::match(argument, key_val_matches, std::regex("^([-]*)([^=]+)[=]([^=]*)$"))) {
				auto positional_counter = size_t(0);
				for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
					auto& parser = parsers.at(parser_index);
					if (!parser.positional) {
						continue;
					}
					if (positional_counter < positional_index) {
						positional_counter += 1;
						continue;
					}
					auto matches = parser.get_matches(argument);
					auto& parser_matches = parser_matches_vector.at(parser_index);
					vector::append(parser_matches, matches);
					positional_index += 1;
					parsed = true;
					break;
				}
			}
			if (!parsed) {
				OVERDRIVE_THROW(exceptions::UnrecognizedArgumentException(argument));
			}
		}
		for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
			auto& parser = parsers.at(parser_index);
			auto& parser_matches = parser_matches_vector.at(parser_index);
			if (parser_matches.size() == 0) {
				if (parser.fallback) {
					auto matches = parser.get_matches(parser.fallback.value());
					vector::append(parser_matches, matches);
				}
			}
			if (parser_matches.size() < parser.min_occurences) {
				OVERDRIVE_THROW(exceptions::BadArgumentOccurencesException(parser.key, parser.min_occurences, parser.max_occurences));
			}
			if (parser_matches.size() > parser.max_occurences) {
				OVERDRIVE_THROW(exceptions::BadArgumentOccurencesException(parser.key, parser.min_occurences, parser.max_occurences));
			}
			if (parser_matches.size() > 0) {
				OVERDRIVE_LOG("Argument \"{}\" got values:", parser.key, parser_matches.size());
				for (auto& match : parser_matches) {
					OVERDRIVE_LOG("\t\"{}\"", match);
				}
				parser.parser(parser_matches);
			}
		}
	}

	auto print(
		const std::vector<Parser>& parsers
	) -> void {
		OVERDRIVE_LOG("Arguments:");
		OVERDRIVE_LOG("");
		auto positional_counter = size_t(0);
		for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
			auto& parser = parsers.at(parser_index);
			OVERDRIVE_LOG("--{}={}", parser.key, parser.format);
			OVERDRIVE_LOG("\t{}", parser.description);
			auto is_optional = parser.min_occurences == 0 || parser.fallback;
			if (parser.max_occurences == 1) {
				OVERDRIVE_LOG("\tValue {} be specified once.", is_optional ? "may" : "must");
			} else {
				OVERDRIVE_LOG("\tValue {} be specified between {} and {} times using \",\" as delimiter.", is_optional ? "may" : "must", parser.min_occurences, parser.max_occurences);
			}
			if (parser.fallback) {
				OVERDRIVE_LOG("\tDefault value is \"{}\".", parser.fallback.value());
			}
			if (parser.positional) {
				OVERDRIVE_LOG("\tMay be specified as positional argument number {}.", positional_counter + 1);
				positional_counter += 1;
			}
			for (auto alias_index = size_t(0); alias_index < parser.aliases.size(); alias_index += 1) {
				auto& alias = parser.aliases.at(alias_index);
				OVERDRIVE_LOG("\tMay be specified using alias \"{}\".", alias);
			}
			OVERDRIVE_LOG("");
		}
	}

	auto sort(
		const std::vector<Parser>& parsers
	) -> std::vector<Parser> {
		auto positional_parsers = vector::filter<Parser>(parsers, [](const Parser& parser, size_t) -> bool_t {
			return parser.positional;
		});
		auto result = vector::filter<Parser>(parsers, [](const Parser& parser, size_t) -> bool_t {
			return !parser.positional;
		});
		// Sort in increasing key order.
		std::sort(result.begin(), result.end(), [](const Parser& one, const Parser& two) -> bool_t {
			return one.key < two.key;
		});
		for (auto& positional_parser : positional_parsers) {
			result.push_back(positional_parser);
		}
		return result;
	};
}
}
