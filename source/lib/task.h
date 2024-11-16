#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "shared.h"

namespace overdrive {
namespace task {
	using namespace shared;

	class Task {
		public:

		std::string key;
		std::string description;
		std::function<void(const std::vector<std::string>& arguments)> runner;

		protected:
	};

	auto run(
		const std::optional<std::string>& key,
		const std::vector<std::string>& arguments,
		const std::vector<Task>& tasks
	) -> void;

	auto print(
		const std::vector<Task>& tasks
	) -> void;

	auto sort(
		std::vector<Task>& tasks
	) -> void;
}
}
