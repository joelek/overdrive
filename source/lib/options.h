#pragma once

#include <optional>
#include <string>
#include <vector>
#include "parser.h"
#include "shared.h"

namespace overdrive {
namespace options {
	using namespace shared;

	class Options {
		public:

		std::string drive;
		std::optional<si_t> read_correction;
		std::optional<std::string> path;
		size_t min_data_passes;
		size_t max_data_passes;
		size_t max_data_retries;
		size_t min_data_copies;
		size_t max_data_copies;
		size_t min_audio_passes;
		size_t max_audio_passes;
		size_t max_audio_retries;
		size_t min_audio_copies;
		size_t max_audio_copies;

		protected:
	};

	auto get_default_parsers(
		Options& options
	) -> std::vector<parser::Parser>;
}
}
