#include "odi.h"

#include <map>

namespace overdrive {
namespace odi {
	auto CompressionMethod::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ NONE, "NONE" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}

	auto Readability::name(
		type value
	) -> const std::string& {
		static const auto names = std::map<type, std::string>({
			{ UNREADABLE, "UNREADABLE" },
			{ READABLE, "READABLE" }
		});
		static const auto fallback = std::string("???");
		auto iterator = names.find(value);
		if (iterator == names.end()) {
			return fallback;
		}
		return iterator->second;
	}
}
}
