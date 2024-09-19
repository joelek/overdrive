#pragma once

#include <string>
#include <vector>

namespace utils {
	namespace string {
		auto split(
			const std::string& string,
			const std::string& delimiter
		) -> std::vector<std::string>;

		auto trim(
			const std::string& string,
			const std::string& characters = " \n\r\t"
		) -> std::string;
	}
}
