#include "path.h"

#include <format>

namespace overdrive {
namespace path {
	Path::operator std::string() {
		return this->fspath.string();
	}

	auto Path::create_directories(
	) const -> void {
		std::filesystem::create_directories(this->fspath);
	}

	auto Path::with_extension(
		const std::string& extension
	) const -> Path {
		auto fspath = this->fspath;
		fspath.replace_extension(extension);
		return {
			fspath
		};
	}

	auto Path::with_filename(
		const std::string& filename
	) const -> Path {
		auto fspath = this->fspath;
		fspath.replace_filename(filename);
		return {
			fspath
		};
	}

	auto Path::with_stem(
		const std::string& stem
	) const -> Path {
		auto fspath = this->fspath;
		fspath.replace_filename(std::format("{}{}", stem, this->fspath.extension().string()));
		return {
			fspath
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
			fspath
		};
	}
}
}
