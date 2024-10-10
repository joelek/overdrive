#include "arguments.h"

namespace overdrive {
namespace arguments {
	auto parse_options_using_parsers(
		const std::vector<std::string>& arguments,
		const std::vector<Parser>& parsers
	) -> void {
		auto positional_index = size_t(0);
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
						break;
					}
				}
			} else {
				auto positional_counter = size_t(0);
				for (auto parser_index = size_t(0); parser_index < parsers.size(); parser_index += 1) {
					auto& parser = parsers.at(parser_index);
					parsed = parser.parse_positional(positional_counter, positional_index, argument);
					if (parsed) {
						break;
					}
				}
			}
			if (!parsed) {
				OVERDRIVE_THROW(exceptions::UnknownArgumentException(argument));
			}
		}
	}
}
}
