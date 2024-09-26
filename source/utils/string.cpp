#include "string.h"

namespace utils {
namespace string {
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
