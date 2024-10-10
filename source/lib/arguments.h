#pragma once

#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include "exceptions.h"
#include "shared.h"
#include "string.h"

namespace overdrive {
namespace arguments {
	using namespace shared;

	class Parser {
		public:

		std::function<bool_t(const std::string& key, const std::string& value)> parse_named;
		std::function<bool_t(size_t& positional_counter, size_t& positional_index, const std::string& argument)> parse_positional;

		protected:
	};

	template <typename A>
	class Argument {
		public:

		std::string key;
		std::string description;
		std::regex regex;
		std::string format;
		bool_t positional;
		std::optional<A> value;
		std::function<A(const std::vector<std::string>& matches)> parser;

		auto make_parser(
		) -> Parser {
			return {
				[&](const std::string& key, const std::string& value) -> bool_t {
					return this->parse_named(key, value);
				},
				[&](size_t& positional_counter, size_t& positional_index, const std::string& argument) -> bool_t {
					return this->parse_positional(positional_counter, positional_index, argument);
				}
			};
		}

		protected:

		auto parse_named(
			const std::string& key,
			const std::string& value
		) -> bool_t {
			if (this->key == key) {
				auto matches = std::vector<std::string>();
				if (string::match(value, matches, this->regex)) {
					this->value = this->parser(matches);
				} else {
					OVERDRIVE_THROW(exceptions::BadArgumentException(this->key, this->format));
				}
				return true;
			}
			return false;
		}

		auto parse_positional(
			size_t& positional_counter,
			size_t& positional_index,
			const std::string& argument
		) -> bool_t {
			if (this->positional) {
				if (positional_counter == positional_index) {
					auto matches = std::vector<std::string>();
					if (string::match(argument, matches, this->format)) {
						this->value = this->parser(matches);
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
	};

	auto parse_options_using_parsers(
		const std::vector<std::string>& arguments,
		const std::vector<Parser>& parsers
	) -> void;
}
}
