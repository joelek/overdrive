#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "detail.h"
#include "shared.h"

namespace overdrive {
namespace command {
	using namespace shared;

	class Command {
		public:

		std::string key;
		std::string description;
		std::function<void(const std::vector<std::string>& arguments, const detail::Detail& detail)> runner;

		protected:
	};

	auto run(
		const std::optional<std::string>& key,
		const std::vector<std::string>& arguments,
		const std::vector<Command>& commands,
		const detail::Detail& detail
	) -> void;

	auto print(
		const std::vector<Command>& commands
	) -> void;

	auto sort(
		std::vector<Command>& commands
	) -> void;
}
}
