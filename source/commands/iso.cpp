#include "iso.h"

#include <optional>
#include <regex>

namespace commands {
	class ISOOptions {
		public:

		std::string drive;

		protected:
	};

	namespace internal {
		auto parse_options(
			const std::vector<std::string>& arguments
		) -> ISOOptions {
			auto drive = std::optional<std::string>();
			for (auto argument_index = size_t(2); argument_index < arguments.size(); argument_index += 1) {
				auto& argument = arguments[argument_index];
				if (false) {
				} else if (argument.find("--drive=") == 0) {
					auto value = argument.substr(sizeof("--drive=") - 1);
					auto format = std::string("^([A-Z])[:]?$");
					auto matches = std::vector<std::string>();
					if (false) {
					} else if (string::match(value, matches, std::regex(format))) {
						drive = matches[1];
					} else {
						throw exceptions::BadArgumentException("drive", format);
					}
				} else {
					throw exceptions::UnknownArgumentException(argument);
				}
			}
			if (!drive) {
				throw exceptions::MissingArgumentException("drive");
			}
			return {
				drive.value()
			};
		}
	}

	auto iso(
		const std::vector<std::string>& arguments
	) -> void {
		auto options = internal::parse_options(arguments);
		printf("%s\n", options.drive.c_str());
	};
}
