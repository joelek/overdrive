#include "iso.h"

#include <format>
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
		const std::vector<std::string>& arguments,
		const std::function<void*(const std::string& drive)>& get_handle,
		const std::function<void(void* handle, byte_t* cdb, size_t cdb_size, byte_t* data, size_t data_size, bool_t write_to_device)>& ioctl
	) -> void {
		try {
			auto options = internal::parse_options(arguments);
			auto handle = get_handle(options.drive);
			auto drive = drive::create_drive(handle, ioctl);
			auto drive_info = drive.read_drive_info();
			fprintf(stderr, "%s\n", std::format("Drive vendor is \"{}\"", string::trim(drive_info.vendor)).c_str());
			fprintf(stderr, "%s\n", std::format("Drive product is \"{}\"", string::trim(drive_info.product)).c_str());


			auto toc = drive.read_full_toc();
			fprintf(stderr, "%s\n", "");
			auto session_type = cdb::get_session_type(toc);
			fprintf(stderr, "%s\n", "");
		} catch (const exceptions::ArgumentException& e) {
			fprintf(stderr, "%s\n", "Arguments:");
			throw e;
		}
	};
}
