#pragma once

#include <regex>
#include <string>
#include <vector>
#include "shared.h"

namespace overdrive {
namespace string {
	using namespace shared;

	auto match(
		const std::string& string,
		std::vector<std::string>& matches,
		const std::regex& regex
	) -> bool_t;

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
