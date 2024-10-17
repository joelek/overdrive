#include "parser.h"

#include <algorithm>
#include <format>
#include "exceptions.h"
#include "string.h"

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
				matches.insert(matches.end(), part_matches.begin(), part_matches.end());
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
					if (parser.key != key) {
						continue;
					}
					auto matches = parser.get_matches(value);
					auto& parser_matches = parser_matches_vector.at(parser_index);
					parser_matches.insert(parser_matches.end(), matches.begin(), matches.end());
					parsed = true;
					break;
				}
			} else {
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
					parser_matches.insert(parser_matches.end(), matches.begin(), matches.end());
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
					parser_matches.insert(parser_matches.end(), matches.begin(), matches.end());
				}
			}
			if (parser_matches.size() < parser.min_occurences) {
				OVERDRIVE_THROW(exceptions::BadArgumentOccurencesException(parser.key, parser.min_occurences, parser.max_occurences));
			}
			if (parser_matches.size() > parser.max_occurences) {
				OVERDRIVE_THROW(exceptions::BadArgumentOccurencesException(parser.key, parser.min_occurences, parser.max_occurences));
			}
			if (parser_matches.size() > 0) {
				fprintf(stderr, "%s\n", std::format("Argument \"{}\" got values:", parser.key, parser_matches.size()).c_str());
				for (auto& match : parser_matches) {
					fprintf(stderr, "%s\n", std::format("\t\"{}\"", match).c_str());
				}
				parser.parser(parser_matches);
			}
		}
	}

	auto print(
		const std::vector<Parser>& parsers
	) -> void {
		fprintf(stderr, "%s\n", std::format("Arguments:").c_str());
		fprintf(stderr, "%s\n", std::format("").c_str());
		auto positional_counter = size_t(0);
		for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
			auto& parser = parsers.at(parser_index);
			fprintf(stderr, "%s\n", std::format("--{}={}", parser.key, parser.format).c_str());
			fprintf(stderr, "%s\n", std::format("\t{}", parser.description).c_str());
			auto is_optional = parser.min_occurences == 0 || parser.fallback;
			if (parser.max_occurences == 1) {
				fprintf(stderr, "%s\n", std::format("\tValue {} be specified once.", is_optional ? "may" : "must").c_str());
			} else {
				fprintf(stderr, "%s\n", std::format("\tValue {} be specified between {} and {} times using \",\" as delimiter.", is_optional ? "may" : "must", parser.min_occurences, parser.max_occurences).c_str());
			}
			if (parser.fallback) {
				fprintf(stderr, "%s\n", std::format("\tDefault value is \"{}\".", parser.fallback.value()).c_str());
			}
			if (parser.positional) {
				fprintf(stderr, "%s\n", std::format("\tMay be specified as positional argument number {}.", positional_counter + 1).c_str());
				positional_counter += 1;
			}
			fprintf(stderr, "%s\n", std::format("").c_str());
		}
	}

	auto sort(
		std::vector<Parser>& parsers
	) -> void {
		// Sort in increasing key order with positional arguments last.
		std::sort(parsers.begin(), parsers.end(), [](const Parser& one, const Parser& two) -> bool_t {
			if (one.positional && !two.positional) {
				return -1;
			}
			if (!one.positional && two.positional) {
				return 1;
			}
			if (one.positional && two.positional) {
				return 0;
			}
			return one.key < two.key;
		});
	};
}
}
