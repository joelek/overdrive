#include "command.h"

#include <algorithm>
#include <format>
#include "app.h"
#include "exceptions.h"
#include "string.h"

namespace overdrive {
namespace command {
	auto run(
		const std::optional<std::string>& key,
		const std::vector<std::string>& arguments,
		const std::vector<Command>& commands
	) -> void {
		if (!key) {
			OVERDRIVE_THROW(exceptions::MissingCommandException());
		}
		for (auto command_index = size_t(0); command_index < commands.size(); command_index += 1) {
			auto& command = commands.at(command_index);
			if (command.key != key.value()) {
				continue;
			}
			return command.runner(arguments);
		}
		OVERDRIVE_THROW(exceptions::UnrecognizedCommandException(key.value()));
	}

	auto print(
		const std::vector<Command>& commands
	) -> void {
		OVERDRIVE_LOG("Commands:");
		OVERDRIVE_LOG("");
		for (auto command_index = size_t(0); command_index < commands.size(); command_index += 1) {
			auto& command = commands.at(command_index);
			OVERDRIVE_LOG("{} {}", string::lower(app::NAME), command.key);
			OVERDRIVE_LOG("\t{}", command.description);
			OVERDRIVE_LOG("");
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
