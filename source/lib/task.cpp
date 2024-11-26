#include "task.h"

#include <algorithm>
#include "app.h"
#include "exceptions.h"
#include "string.h"

namespace overdrive {
namespace task {
	auto run(
		const std::optional<std::string>& key,
		const std::vector<std::string>& arguments,
		const std::vector<Task>& tasks
	) -> void {
		if (!key) {
			OVERDRIVE_THROW(exceptions::MissingTaskException());
		}
		for (auto task_index = size_t(0); task_index < tasks.size(); task_index += 1) {
			auto& task = tasks.at(task_index);
			if (task.key != key.value()) {
				continue;
			}
			return task.runner(arguments);
		}
		OVERDRIVE_THROW(exceptions::UnrecognizedTaskException(key.value()));
	}

	auto print(
		const std::vector<Task>& tasks
	) -> void {
		OVERDRIVE_LOG("Tasks:");
		OVERDRIVE_LOG("");
		for (auto task_index = size_t(0); task_index < tasks.size(); task_index += 1) {
			auto& task = tasks.at(task_index);
			OVERDRIVE_LOG("{} {}", string::lower(app::NAME), task.key);
			OVERDRIVE_LOG("\t{}", task.description);
			OVERDRIVE_LOG("");
		}
	}

	auto sort(
		std::vector<Task>& tasks
	) -> void {
		// Sort in increasing key order.
		std::sort(tasks.begin(), tasks.end(), [](const Task& one, const Task& two) -> bool_t {
			return one.key < two.key;
		});
	};
}
}
