#include "arguments.h"

#include <format>
#include "exceptions.h"
#include "string.h"

namespace overdrive {
namespace arguments {
	auto Parser::parse_named(
		const std::string& key,
		const std::string& value
	) const -> bool_t {
		if (this->key == key) {
			auto matches = std::vector<std::string>();
			if (string::match(value, matches, this->regex)) {
				this->parser(matches);
			} else {
				OVERDRIVE_THROW(exceptions::BadArgumentException(this->key, this->format));
			}
			return true;
		}
		return false;
	}

	auto Parser::parse_positional(
		size_t& positional_counter,
		size_t& positional_index,
		const std::string& argument
	) const -> bool_t {
		if (this->positional) {
			if (positional_counter == positional_index) {
				auto matches = std::vector<std::string>();
				if (string::match(argument, matches, this->regex)) {
					this->parser(matches);
				} else {
					OVERDRIVE_THROW(exceptions::BadArgumentException(this->key, this->format));
				}
				positional_index += 1;
				return true;
			}
			positional_counter += 1;
		}
		return false;
	}

	auto parse(
		const std::vector<std::string>& arguments,
		const std::vector<Parser>& parsers
	) -> void {
		auto positional_index = size_t(0);
		auto states = std::vector<bool_t>(parsers.size());
		for (auto argument_index = size_t(0); argument_index < arguments.size(); argument_index += 1) {
			auto& argument = arguments[argument_index];
			auto parsed = false;
			auto matches = std::vector<std::string>();
			if (string::match(argument, matches, std::regex("^[-][-]([^=]+)[=]([^=]+)$"))) {
				auto& key = matches.at(0);
				auto& value = matches.at(1);
				for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
					auto& parser = parsers.at(parser_index);
					parsed = parser.parse_named(key, value);
					if (parsed) {
						states.at(parser_index) = true;
						break;
					}
				}
			} else {
				auto positional_counter = size_t(0);
				for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
					auto& parser = parsers.at(parser_index);
					parsed = parser.parse_positional(positional_counter, positional_index, argument);
					if (parsed) {
						states.at(parser_index) = true;
						break;
					}
				}
			}
			if (!parsed) {
				OVERDRIVE_THROW(exceptions::UnknownArgumentException(argument));
			}
		}
		for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
			if (!states.at(parser_index)) {
				auto& parser = parsers.at(parser_index);
				if (parser.fallback) {
					parser.parse_named(parser.key, parser.fallback.value());
				} else {
					if (parser.required) {
						OVERDRIVE_THROW(exceptions::MissingArgumentException(parser.key));
					}
				}
			}
		}
	}

	auto print(
		const std::vector<Parser>& parsers
	) -> void {
		fprintf(stderr, "%s\n", std::format("Arguments:").c_str());
		fprintf(stderr, "%s\n", std::format("").c_str());
		for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
			auto& parser = parsers.at(parser_index);
			fprintf(stderr, "%s\n", std::format("--{}={}", parser.key, parser.format).c_str());
			fprintf(stderr, "%s\n", std::format("\t{}", parser.description).c_str());
			fprintf(stderr, "%s\n", std::format("\t{}", parser.fallback ? std::format("Optional with \"{}\" as default.", parser.fallback.value()) : parser.required ? "Required." : "Optional with dynamic default.").c_str());
			fprintf(stderr, "%s\n", std::format("").c_str());
		}
	}
}
}
