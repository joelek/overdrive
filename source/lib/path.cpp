#include "path.h"

#include <filesystem>
#include <format>

namespace overdrive {
namespace path {
	Path::operator std::string() {
		auto fspath = std::filesystem::path(this->directory) / std::format("{}{}", this->stem, this->extension);
		return fspath.string();
	}

	auto Path::with_stem(
		const std::string& stem
	) const -> Path {
		return {
			this->directory,
			stem,
			this->extension
		};
	}

	auto Path::with_extension(
		const std::string& extension
	) const -> Path {
		return {
			this->directory,
			this->stem,
			extension
		};
	}

	auto create_path(
		const std::optional<std::string>& path
	) -> Path {
		auto fspath = std::filesystem::path(path.value_or(""));
		if (!fspath.has_stem() || fspath.stem().string().starts_with(".")) {
			fspath.replace_filename(DEFAULT_STEM);
		}
		fspath = std::filesystem::weakly_canonical(std::filesystem::current_path() / fspath);
		return {
			fspath.parent_path().string(),
			fspath.stem().string(),
			fspath.extension().string()
		};
	}
}
}
