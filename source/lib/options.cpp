#include "options.h"

#include <cstdlib>
#include <regex>

namespace overdrive {
namespace options {
	auto get_default_parsers(
		Options& options
	) -> std::vector<parser::Parser> {
		auto parsers = std::vector<parser::Parser>();
		parsers.push_back(parser::Parser({
			"drive",
			{},
			"Specify which drive to read from.",
			std::regex("^([A-Z])[:]?$"),
			"string",
			true,
			std::optional<std::string>(),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.drive = matches.at(0);
			}
		}));
		parsers.push_back(parser::Parser({
			"path",
			{},
			"Specify which path to write to.",
			std::regex("^(.+)$"),
			"string",
			true,
			std::optional<std::string>(),
			0,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.path = matches.at(0);
			}
		}));
		parsers.push_back(parser::Parser({
			"read-correction",
			{},
			"Specify read offset correction in number of samples.",
			std::regex("^([+-]?(?:[0-9]|[1-9][0-9]+))$"),
			"integer",
			false,
			std::optional<std::string>(),
			0,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.read_correction = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"min-data-passes",
			{ "min-passes" },
			"Specify the minimum number of read passes for data tracks.",
			std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("1"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.min_data_passes = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"max-data-passes",
			{ "max-passes" },
			"Specify the maximum number of read passes for data tracks.",
			std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("1"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.max_data_passes = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"max-data-retries",
			{ "max-retries" },
			"Specify the maximum number of read retires for data tracks.",
			std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("16"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.max_data_retries = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"min-data-copies",
			{ "min-copies" },
			"Specify the minimum acceptable number of identical copies for data tracks.",
			std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("0"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.min_data_copies = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"max-data-copies",
			{ "max-copies" },
			"Specify the maximum acceptable number of identical copies for data tracks.",
			std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("1"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.max_data_copies = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"min-audio-passes",
			{ "min-passes" },
			"Specify the minimum number of read passes for audio tracks.",
			std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("2"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.min_audio_passes = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"max-audio-passes",
			{ "max-passes" },
			"Specify the maximum number of read passes for audio tracks.",
			std::regex("^([1-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("8"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.max_audio_passes = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"max-audio-retries",
			{ "max-retries" },
			"Specify the maximum number of read retires for audio tracks.",
			std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("255"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.max_audio_retries = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"min-audio-copies",
			{ "min-copies" },
			"Specify the minimum acceptable number of identical copies for audio tracks.",
			std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("1"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.min_audio_copies = std::atoi(matches.at(0).c_str());
			}
		}));
		parsers.push_back(parser::Parser({
			"max-audio-copies",
			{ "max-copies" },
			"Specify the maximum acceptable number of identical copies for audio tracks.",
			std::regex("^([0-9]|[1-9][0-9]|[1][0-9][0-9]|[2][0-4][0-9]|[2][5][0-5])$"),
			"integer",
			false,
			std::optional<std::string>("2"),
			1,
			1,
			[&](const std::vector<std::string>& matches) -> void {
				options.max_audio_copies = std::atoi(matches.at(0).c_str());
			}
		}));
		return parsers;
	}
}
}
