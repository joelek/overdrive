#pragma once

#include <optional>
#include <string>
#include "shared.h"

namespace overdrive {
namespace path {
	using namespace shared;

	const auto DEFAULT_STEM = std::string("image");

	class Path {
		public:

		std::string directory;
		std::string stem;
		std::string extension;

		operator std::string();

		auto with_stem(
			const std::string& stem
		) const -> Path;

		auto with_extension(
			const std::string& extension
		) const -> Path;

		protected:
	};

	auto create_path(
		const std::optional<std::string>& path
	) -> Path;
}
}
