#include <algorithm>
#include <format>
#include <optional>
#include <string>
#include <vector>
#include "../lib/overdrive.h"
#include "commands/cue.h"
#include "commands/iso.h"
#include "commands/mds.h"
#include "commands/odi.h"

using namespace overdrive;
using namespace shared;

auto main(
	si_t argc,
	ch08_t** argv
) -> si_t {
	auto logger = logger::Logger("stderr");
	try {
		auto arguments = std::vector<std::string>(argv + std::min<size_t>(2, argc), argv + argc);
		auto command = argc < 2 ? std::optional<std::string>() : std::string(argv[1]);
		auto commands = std::vector<command::Command>();
		commands.push_back(command::Command({
			"cue",
			"Backup discs using the BIN/CUE image format.",
			commands::cue
		}));
		commands.push_back(command::Command({
			"iso",
			"Backup discs using the ISO image format.",
			commands::iso
		}));
		commands.push_back(command::Command({
			"mds",
			"Backup discs using the MDF/MDS image format.",
			commands::mds
		}));
		commands.push_back(command::Command({
			"odi",
			"Backup discs using the ODI image format.",
			commands::odi
		}));
		command::sort(commands);
		try {
			auto start_ms = time::get_time_ms();
			command::run(command, arguments, commands);
			auto duration_ms = time::get_duration_ms(start_ms);
			logger.log("Command execution took {} ms.", duration_ms);
		} catch (const exceptions::CommandException& e) {
			command::print(commands);
			throw;
		}
		logger.log("Program completed successfully.");
		return EXIT_SUCCESS;
	} catch (const std::exception& e) {
		logger.log("{}", e.what());
	} catch (...) {}
	logger.log("Program did not complete successfully!");
	return EXIT_FAILURE;
}
