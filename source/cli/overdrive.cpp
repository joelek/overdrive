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
			auto start_ms = time::get_time_ms();
			command::run(command, arguments, commands, {
				get_handle,
				pass_through_direct
			});
			auto duration_ms = time::get_duration_ms(start_ms);
			fprintf(stderr, "%s\n", std::format("Command execution took {} ms.", duration_ms).c_str());
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
