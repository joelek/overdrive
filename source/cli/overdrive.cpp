#include <algorithm>
#include <format>
#include <optional>
#include <string>
#include <vector>
#include "../lib/overdrive.h"
#include "tasks/cue.h"
#include "tasks/iso.h"
#include "tasks/mds.h"
#include "tasks/odi.h"

using namespace overdrive;
using namespace shared;

auto main(
	si_t argc,
	ch08_t** argv
) -> si_t {
	try {
		auto arguments = std::vector<std::string>(argv + std::min<size_t>(2, argc), argv + argc);
		auto task = argc < 2 ? std::optional<std::string>() : std::string(argv[1]);
		auto tasks = std::vector<task::Task>();
		tasks.push_back(task::Task({
			"cue",
			"Backup discs using the BIN/CUE image format.",
			tasks::cue
		}));
		tasks.push_back(task::Task({
			"iso",
			"Backup discs using the ISO image format.",
			tasks::iso
		}));
		tasks.push_back(task::Task({
			"mds",
			"Backup discs using the MDF/MDS image format.",
			tasks::mds
		}));
		tasks.push_back(task::Task({
			"odi",
			"Backup discs using the ODI image format.",
			tasks::odi
		}));
		task::sort(tasks);
		try {
			auto start_ms = time::get_time_ms();
			task::run(task, arguments, tasks);
			auto duration_ms = time::get_duration_ms(start_ms);
			OVERDRIVE_LOG("Task execution took {} ms.", duration_ms);
		} catch (const exceptions::TaskException& e) {
			task::print(tasks);
			throw;
		}
		OVERDRIVE_LOG("Program completed successfully.");
		return EXIT_SUCCESS;
	} catch (const std::exception& e) {
		OVERDRIVE_LOG("{}", e.what());
	} catch (...) {}
	OVERDRIVE_LOG("Program did not complete successfully!");
	return EXIT_FAILURE;
}
