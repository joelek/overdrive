#include "memory.h"

#include <cstring>

namespace overdrive {
namespace memory {
	auto test(
		const void* pointer,
		size_t size,
		byte_t value
	) -> bool_t {
		auto buffer = reinterpret_cast<const byte_t*>(pointer);
		return (buffer[0] == value) && std::memcmp(buffer, buffer + 1, size - 1) == 0;
	}
}
}
