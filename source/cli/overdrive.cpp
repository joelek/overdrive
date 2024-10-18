#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <string>
#include <cstdint>
#include <windows.h>
#include <winioctl.h>
#include <ntddcdrm.h>
#include <ntddscsi.h>
#include <fcntl.h>
#include <io.h>
#include <optional>
#include <memory>
#include <algorithm>

#include "commands/cue.h"
#include "commands/iso.h"
#include "commands/mds.h"
#include "commands/odi.h"
#include "../lib/overdrive.h"

using namespace overdrive;
using namespace shared;

#ifdef DEBUG
	#define WINAPI_CHECK_STATUS() \
		{ \
			auto status = GetLastError(); \
			if (status != ERROR_SUCCESS) { \
				fprintf(stderr, "[%s:%u]: Unexpected WINAPI status %lu!\n", __FILE__, __LINE__, status); \
				throw EXIT_FAILURE; \
			} \
		}
#else
	#define WINAPI_CHECK_STATUS() \
		{ \
			auto status = GetLastError(); \
			if (status != ERROR_SUCCESS) { \
				fprintf(stderr, "Unexpected WINAPI status %lu!\n", status); \
				throw EXIT_FAILURE; \
			} \
		}
#endif

struct SPTDWithSenseBuffer {
	SCSI_PASS_THROUGH_DIRECT sptd;
	byte_t sense[255];
};

auto pass_through_direct(
	void* handle,
	byte_t* cdb,
	size_t cdb_size,
	byte_t* data,
	size_t data_size,
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
	auto outcome = DeviceIoControl(
		handle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptd_sense,
		sizeof(sptd_sense),
		&sptd_sense,
		sizeof(sptd_sense),
		nullptr,
		nullptr
	);
	WINAPI_CHECK_STATUS();
	if (!outcome) {
		throw EXIT_FAILURE;
	}
	/*
	if (sptd_sense.sense[0] == (byte_t)sense::ResponseCodes::FIXED_CURRENT) {
		auto& sense = *reinterpret_cast<sense::FixedFormat*>(sptd_sense.sense);
		fprintf(stderr, "[WARNING] Sense info 0x%.1X 0x%.2X 0x%.2X!\n", (unsigned)sense.sense_key, sense.additional_sense_code, sense.additional_sense_code_qualifier);
	}
	*/
	return sptd_sense.sptd.ScsiStatus;
}

auto get_timestamp_ms()
-> uint64_t {
	auto current_time_filetime = FILETIME();
	SetLastError(ERROR_SUCCESS);
	GetSystemTimeAsFileTime(&current_time_filetime);
	WINAPI_CHECK_STATUS();
	auto current_time_integer = ULARGE_INTEGER();
	current_time_integer.HighPart = current_time_filetime.dwHighDateTime;
	current_time_integer.LowPart = current_time_filetime.dwLowDateTime;
	auto epoch_start = SYSTEMTIME();
	epoch_start.wYear = 1970;
	epoch_start.wMonth = 1;
	epoch_start.wDay = 1;
	epoch_start.wDayOfWeek = 4;
	epoch_start.wHour = 0;
	epoch_start.wMinute = 0;
	epoch_start.wSecond = 0;
	epoch_start.wMilliseconds = 0;
	auto epoch_start_filetime = FILETIME();
	SetLastError(ERROR_SUCCESS);
	SystemTimeToFileTime(&epoch_start, &epoch_start_filetime);
	WINAPI_CHECK_STATUS();
	auto epoch_start_integer = ULARGE_INTEGER();
	epoch_start_integer.HighPart = epoch_start_filetime.dwHighDateTime;
	epoch_start_integer.LowPart = epoch_start_filetime.dwLowDateTime;
	auto current_time_ms = (current_time_integer.QuadPart - epoch_start_integer.QuadPart) / 10000;
	return current_time_ms;
}

auto get_handle(
	const std::string& drive
) -> void* {
	auto filename = std::format("\\\\.\\{}:", drive);
	SetLastError(ERROR_SUCCESS);
	auto handle = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	WINAPI_CHECK_STATUS();
	return handle;
}











































auto main(
	si_t argc,
	ch08_t** argv
) -> si_t {
	try {
		auto arguments = std::vector<std::string>(argv + std::min<size_t>(2, argc), argv + argc);
		auto command = argc < 2 ? std::optional<std::string>() : std::string(argv[1]);
		auto commands = std::vector<command::Command>();
		commands.push_back(command::Command({
			"cue",
			"Backup discs using the BIN/CUE image format.",
			commands::cue
		}));
		commands.push_back(command::Command({
			"iso",
			"Backup discs using the ISO image format.",
			commands::iso
		}));
		commands.push_back(command::Command({
			"mds",
			"Backup discs using the MDF/MDS image format.",
			commands::mds
		}));
		commands.push_back(command::Command({
			"odi",
			"Backup discs using the ODI image format.",
			commands::odi
		}));
		command::sort(commands);
		try {
			command::run(command, arguments, commands, {
				get_handle,
				pass_through_direct
			});
		} catch (const exceptions::CommandException& e) {
			command::print(commands);
			throw;
		}
		fprintf(stderr, "%s\n", std::format("Program completed successfully.").c_str());
		return EXIT_SUCCESS;
	} catch (const std::exception& e) {
		fprintf(stderr, "%s\n", e.what());
	} catch (...) {}
	fprintf(stderr, "%s\n", std::format("Program did not complete successfully!").c_str());
	return EXIT_FAILURE;
}
