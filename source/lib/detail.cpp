#include "detail.h"

#include <cstring>
#include <format>
#include <regex>
#include <vector>
#include "exceptions.h"
#include "string.h"

#if _WIN32 || _WIN64

#include <errhandlingapi.h>
#include <fileapi.h>
#include <ioapiset.h>
#include <ntddscsi.h>
#include <winerror.h>
#include <winioctl.h>

namespace overdrive {
namespace detail {
	namespace internal {
	namespace {
		struct SPTDWithSenseBuffer {
			SCSI_PASS_THROUGH_DIRECT sptd;
			byte_t sense[255];
		};

		auto get_handle(
			const std::string& drive
		) -> void* {
			auto matches = std::vector<std::string>();
			if (!string::match(drive, matches, std::regex("^([A-Z])[:]?$"))) {
				OVERDRIVE_THROW(exceptions::BadArgumentFormatException(drive, "letter"));
			}
			auto filename = std::format("\\\\.\\{}:", matches.at(0));
			SetLastError(ERROR_SUCCESS);
			auto handle = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			auto status = GetLastError();
			if (status != ERROR_SUCCESS) {
				OVERDRIVE_THROW(exceptions::WindowsException(status));
			}
			return handle;
		}

		auto ioctl(
			void* handle,
			byte_t* cdb,
			size_t cdb_size,
			byte_t* data,
			size_t data_size,
			pointer<array<255, byte_t>> sense,
			bool_t write_to_device
		) -> byte_t {
			auto sptd_sense = SPTDWithSenseBuffer();
			sptd_sense.sptd.Length = sizeof(sptd_sense.sptd);
			sptd_sense.sptd.CdbLength = cdb_size;
			sptd_sense.sptd.DataIn = write_to_device ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN;
			sptd_sense.sptd.TimeOutValue = 10;
			sptd_sense.sptd.DataBuffer = data;
			sptd_sense.sptd.DataTransferLength = data_size;
			sptd_sense.sptd.SenseInfoLength = sizeof(sptd_sense.sense);
			sptd_sense.sptd.SenseInfoOffset = offsetof(SPTDWithSenseBuffer, sense);
			std::memcpy(sptd_sense.sptd.Cdb, cdb, cdb_size);
			SetLastError(ERROR_SUCCESS);
			DeviceIoControl(
				handle,
				IOCTL_SCSI_PASS_THROUGH_DIRECT,
				&sptd_sense,
				sizeof(sptd_sense),
				&sptd_sense,
				sizeof(sptd_sense),
				nullptr,
				nullptr
			);
			auto status = GetLastError();
			if (status != ERROR_SUCCESS) {
				OVERDRIVE_THROW(exceptions::WindowsException(status));
			}
			if (sense != nullptr) {
				std::memcpy(*sense, sptd_sense.sense, sizeof(*sense));
			}
			return sptd_sense.sptd.ScsiStatus;
		}
	}
	}

	auto create_detail(
	) -> Detail {
		auto get_handle = internal::get_handle;
		auto ioctl = internal::ioctl;
		return {
			get_handle,
			ioctl
		};
	}
}
}

#else

#error Implementation in "source/lib/detail.cpp" is missing for target platform.

#endif
