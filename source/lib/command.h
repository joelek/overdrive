#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "shared.h"

namespace overdrive {
namespace command {
	using namespace shared;

	class Detail {
		public:

		std::function<void*(const std::string& drive)> get_handle;
		std::function<byte_t(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)> ioctl;

		protected:
	};

	class Command {
		public:

		std::string key;
		std::string description;
		std::function<void(const std::vector<std::string>& arguments, const Detail& detail)> runner;

		protected:
	};

	auto run(
		const std::optional<std::string>& key,
		const std::vector<std::string>& arguments,
		const std::vector<Command>& commands,
		const Detail& detail
	) -> void;

	auto print(
		const std::vector<Command>& commands
	) -> void;

	auto sort(
		std::vector<Command>& commands
	) -> void;
}
}
