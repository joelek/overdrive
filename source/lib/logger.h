#pragma once

#include <cstdio>
#include <format>
#include <string>
#include "shared.h"

namespace overdrive {
namespace logger {
	using namespace shared;

	class Logger {
		public:

		Logger(
			const std::string& path
		);

		template <typename... A>
		auto log(
			const std::string& format,
			A&&... arguments
		) const -> void;

		protected:

		FILE* handle;
	};

	template <typename... A>
	auto Logger::log(
		const std::string& format,
		A&&... arguments
	) const -> void {
		fprintf(this->handle, "%s\n", std::vformat(format, std::make_format_args(arguments...)).c_str());
	}
}
}
