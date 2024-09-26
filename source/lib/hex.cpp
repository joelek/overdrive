#include "hex.h"

#include <format>

namespace overdrive {
namespace hex {
	auto create_hex_dump(
		const byte_t* bytes,
		size_t size
	) -> std::string {
		auto rows = (size + 15) / 16;
		auto offset = 0;
		auto remaining_bytes = size;
		auto string = std::string();
		for (auto r = size_t(0); r < rows; r += 1) {
			string += std::format("{:0>8X}: ", offset);
			auto cols = remaining_bytes <= 16 ? remaining_bytes : 16;
			for (auto c = size_t(0); c < cols; c += 1) {
				auto byte = bytes[offset + c];
				string += std::format("{:0>2X} ", byte);
			}
			for (auto c = cols; c < 16; c += 1) {
				string += "   ";
			}
			for (auto c = size_t(0); c < cols; c += 1) {
				auto byte = bytes[offset + c];
				string += std::format("{:c}", byte >= 32 && byte <= 127 ? byte : '.');
			}
			for (auto c = cols; c < 16; c += 1) {
				string += " ";
			}
			string += "\n";
			offset += cols;
			remaining_bytes -= cols;
		}
		return string;
	}
}
}
