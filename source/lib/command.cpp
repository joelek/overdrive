#include "command.h"

#include <algorithm>
#include <format>
#include "app.h"
#include "exceptions.h"

namespace overdrive {
namespace command {
	auto run(
		const std::optional<std::string>& key,
		const std::vector<std::string>& arguments,
		const std::vector<Command>& commands,
		const Detail& detail
	) -> void {
		if (!key) {
			OVERDRIVE_THROW(exceptions::MissingCommandException());
		}
		for (auto command_index = size_t(0); command_index < commands.size(); command_index += 1) {
			auto& command = commands.at(command_index);
			if (command.key != key.value()) {
				continue;
			}
			return command.runner(arguments, detail);
		}
		OVERDRIVE_THROW(exceptions::UnrecognizedCommandException(key.value()));
	}

	auto print(
		const std::vector<Command>& commands
	) -> void {
		fprintf(stderr, "%s\n", std::format("Commands:").c_str());
		fprintf(stderr, "%s\n", std::format("").c_str());
		for (auto command_index = size_t(0); command_index < commands.size(); command_index += 1) {
			auto& command = commands.at(command_index);
			fprintf(stderr, "%s\n", std::format("{} {}", app::NAME, command.key).c_str());
			fprintf(stderr, "%s\n", std::format("\t{}", command.description).c_str());
			fprintf(stderr, "%s\n", std::format("").c_str());
		}
	}

	auto sort(
		std::vector<Command>& commands
	) -> void {
		// Sort in increasing key order.
		std::sort(commands.begin(), commands.end(), [](const Command& one, const Command& two) -> bool_t {
			return one.key < two.key;
		});
	};
}
}
