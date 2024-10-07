#include "string.h"

namespace overdrive {
namespace string {
	auto join(
		const std::vector<std::string>& parts,
		const std::string& glue
	) -> std::string {
		auto string = std::string();
		for (auto part_index = size_t(0); part_index < parts.size(); part_index += 1) {
			auto& part = parts.at(part_index);
			if (part_index > 0) {
				string += glue;
			}
			string += part;
		}
		return string;
	}

	auto match(
		const std::string& string,
		std::vector<std::string>& matches,
		const std::regex& regex
	) -> bool_t {
		matches.clear();
		auto smatch = std::smatch();
		if (std::regex_search(string, smatch, regex)) {
			for (auto match_index = size_t(0); match_index < smatch.size(); match_index += 1) {
				matches.push_back(smatch[match_index]);
			}
			return true;
		} else {
			return false;
		}
	}

	auto split(
		const std::string& string,
		const std::string& delimiter
	) -> std::vector<std::string> {
		auto parts = std::vector<std::string>();
		auto offset = size_t(0);
		while (true) {
			auto offset_found = string.find(delimiter, offset);
			auto part = string.substr(offset, offset_found - offset);
			parts.push_back(part);
			if (offset_found >= string.size()) {
				break;
			}
			offset = offset_found + delimiter.size();
		}
		return parts;
	}

	auto trim(
		const std::string& string,
		const std::string& characters
	) -> std::string {
		auto start = string.find_first_not_of(characters);
		if (start >= string.size()) {
			return "";
		}
		auto end = string.find_last_not_of(characters);
		return string.substr(start, end - start + 1);
	}
}
}
