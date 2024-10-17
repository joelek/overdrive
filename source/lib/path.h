#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include "shared.h"

namespace overdrive {
namespace path {
	using namespace shared;

	const auto DEFAULT_STEM = std::string("image");

	class Path {
		public:

		std::filesystem::path fspath;

		operator std::string();

		auto create_directories(
		) const -> void;

		auto with_extension(
			const std::string& extension
		) const -> Path;

		auto with_filename(
			const std::string& filename
		) const -> Path;

		auto with_stem(
			const std::string& stem
		) const -> Path;

		protected:
	};

	auto create_path(
		const std::optional<std::string>& path
	) -> Path;
}
}
