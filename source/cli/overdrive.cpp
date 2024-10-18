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
	}/*
	if (sptd_sense.sense[0] == (byte_t)sense::ResponseCodes::FIXED_CURRENT) {
		auto& sense = *reinterpret_cast<sense::FixedFormat*>(sptd_sense.sense);
		fprintf(stderr, "[WARNING] Sense info 0x%.1X 0x%.2X 0x%.2X!\n", (unsigned)sense.sense_key, sense.additional_sense_code, sense.additional_sense_code_qualifier);
	} */
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
	auto filename = std::string("\\\\.\\") + drive + ":";
	SetLastError(ERROR_SUCCESS);
	auto handle = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	WINAPI_CHECK_STATUS();
	return handle;
}


































class MDSImageFormat {
	public:

	auto write_index(const drive::Drive& drive, const cdb::ReadTOCResponseNormalTOC& toc, const cdb::ReadTOCResponseFullTOC& toc_ex, bool subchannels, const std::vector<int>& bad_sector_numbers, const std::vector<unsigned int>& track_pregap_sectors_list, const std::vector<unsigned int>& track_length_sectors_list) -> void {
		auto &first_track = toc.entries[toc.header.first_track_or_session_number - 1];
		auto &lead_out_track = toc.entries[toc.header.last_track_or_session_number + 1 - 1];
		auto track_count = toc.header.last_track_or_session_number - toc.header.first_track_or_session_number + 1;
		auto sector_count = cd::get_sector_from_address(lead_out_track.track_start_address) - cd::get_sector_from_address(first_track.track_start_address);
		auto absolute_offset_to_track_table_entry = sizeof(mds::FormatHeader) + sizeof(mds::SessionHeader) + 3 * sizeof(mds::EntryTypeA) + track_count * sizeof(mds::EntryTypeB) + sizeof(mds::TrackTableHeader);
		auto absolute_offset_to_file_table_header = absolute_offset_to_track_table_entry + track_count * sizeof(mds::TrackTableEntry);
		auto absolute_offset_to_file_table_entry = absolute_offset_to_file_table_header + sizeof(mds::FileTableHeader);
		auto absolute_offset_to_footer = absolute_offset_to_file_table_entry + sizeof(mds::FileTableEntry);
		auto absolute_offset_to_bad_sectors_table_header = absolute_offset_to_footer + sizeof(mds::Footer);
		auto format_header = mds::FormatHeader();
		format_header.absolute_offset_to_footer = bad_sector_numbers.size() == 0 ? 0 : absolute_offset_to_footer;
		if (fwrite(&format_header, sizeof(format_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing format header!\n");
			throw EXIT_FAILURE;
		}
		auto session_header = mds::SessionHeader();
		session_header.sectors_on_disc = sector_count;
		session_header.entry_count = track_count + 3;
		session_header.entry_count_type_a = 3;
		session_header.last_track = track_count;
		if (fwrite(&session_header, sizeof(session_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing disc header!\n");
			throw EXIT_FAILURE;
		}
		auto toc_ex_length = byteswap::byteswap16(toc_ex.header.data_length_be);
		auto toc_ex_tracks = (toc_ex_length - sizeof(toc_ex.header.data_length_be)) / sizeof(cdb::ReadTOCResponseFullTOCEntry);
		auto first_sector_on_disc = 0;
		auto mdf_byte_offset = 0;
		auto track_number = toc.header.first_track_or_session_number;
		for (auto toc_ex_track_index = 0u; toc_ex_track_index < toc_ex_tracks; toc_ex_track_index += 1) {
			auto &current_track = toc_ex.entries[toc_ex_track_index];
			if (current_track.point >= 0xA0) {
				auto current_track_entry = mds::EntryTypeA();
				current_track_entry.track_mode = mds::TrackMode::NONE;
				current_track_entry.track_mode_flags = mds::TrackModeFlags::UNKNOWN_0;
				std::memcpy((UCHAR*)&current_track_entry + offsetof(mds::EntryTypeA, subchannel_mode), &current_track, sizeof(current_track));
				current_track_entry.subchannel_mode = subchannels ? mds::SubchannelMode::INTERLEAVED_96 : mds::SubchannelMode::NONE;
				if (fwrite(&current_track_entry, sizeof(current_track_entry), 1, target_handle_mds) != 1) {
					fprintf(stderr, "Error writing current track entry!\n");
					throw EXIT_FAILURE;
				}
			} else {
				auto current_track_entry = mds::EntryTypeB();
				auto current_track_type = drive.determine_track_type(toc_ex, toc_ex_track_index);
				auto current_track_mode = mds::get_track_mode(current_track_type);
				current_track_entry.track_mode = current_track_mode;
				current_track_entry.track_mode_flags = current_track_mode == mds::TrackMode::MODE2_FORM1 || current_track_mode == mds::TrackMode::MODE2_FORM2 ? mds::TrackModeFlags::UNKNOWN_E : mds::TrackModeFlags::UNKNOWN_A;
				std::memcpy((UCHAR*)&current_track_entry + offsetof(mds::EntryTypeA, subchannel_mode), &current_track, sizeof(current_track));
				current_track_entry.subchannel_mode = subchannels ? mds::SubchannelMode::INTERLEAVED_96 : mds::SubchannelMode::NONE;
				current_track_entry.sector_length = subchannels ? cd::SECTOR_LENGTH + cd::SUBCHANNELS_LENGTH : cd::SECTOR_LENGTH;
				current_track_entry.first_sector_on_disc = first_sector_on_disc;
				current_track_entry.mdf_byte_offset = mdf_byte_offset;
				current_track_entry.absolute_offset_to_track_table_entry = absolute_offset_to_track_table_entry + (track_number - toc.header.first_track_or_session_number) * sizeof(mds::TrackTableEntry);
				current_track_entry.absolute_offset_to_file_table_header = absolute_offset_to_file_table_header;
				if (fwrite(&current_track_entry, sizeof(current_track_entry), 1, target_handle_mds) != 1) {
					fprintf(stderr, "Error writing current track entry!\n");
					throw EXIT_FAILURE;
				}
				auto current_track_length_sectors = track_length_sectors_list.at(track_number - toc.header.first_track_or_session_number);
				auto next_track_pregap_sectors = track_number == toc.header.last_track_or_session_number ? 0 : track_pregap_sectors_list.at(track_number + 1 - toc.header.first_track_or_session_number);
				first_sector_on_disc += current_track_length_sectors + next_track_pregap_sectors;
				mdf_byte_offset += current_track_length_sectors * current_track_entry.sector_length;
				track_number += 1;
			}
		}
		auto track_table_header = mds::TrackTableHeader();
		if (fwrite(&track_table_header, sizeof(track_table_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing track table header!\n");
			throw EXIT_FAILURE;
		}
		for (auto i = toc.header.first_track_or_session_number; i <= toc.header.last_track_or_session_number; i += 1) {
			auto track_table_entry = mds::TrackTableEntry();
			track_table_entry.pregap_sectors = track_pregap_sectors_list.at(i - toc.header.first_track_or_session_number);
			track_table_entry.length_sectors = track_length_sectors_list.at(i - toc.header.first_track_or_session_number);
			if (fwrite(&track_table_entry, sizeof(track_table_entry), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing track table entry!\n");
				throw EXIT_FAILURE;
			}
		}
		auto file_table_header = mds::FileTableHeader();
		file_table_header.absolute_offset_to_file_table_entry = absolute_offset_to_file_table_entry;
		if (fwrite(&file_table_header, sizeof(file_table_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing file table header!\n");
			throw EXIT_FAILURE;
		}
		auto file_table_entry = mds::FileTableEntry();
		if (fwrite(&file_table_entry, sizeof(file_table_entry), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing file table entry!\n");
			throw EXIT_FAILURE;
		}
		if (bad_sector_numbers.size() > 0) {
			auto footer = mds::Footer();
			footer.absolute_offset_to_bad_sectors_table_header = absolute_offset_to_bad_sectors_table_header;
			if (fwrite(&footer, sizeof(footer), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing footer!\n");
				throw EXIT_FAILURE;
			}
			auto bad_sector_table_header = mds::BadSectorTableHeader();
			bad_sector_table_header.bad_sector_count = bad_sector_numbers.size();
			if (fwrite(&bad_sector_table_header, sizeof(bad_sector_table_header), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing bad sector table header!\n");
				throw EXIT_FAILURE;
			}
			for (auto bad_sector_number : bad_sector_numbers) {
				auto bad_sector_table_entry = mds::BadSectorTableEntry();
				bad_sector_table_entry.bad_sector_index = bad_sector_number;
				if (fwrite(&bad_sector_table_entry, sizeof(bad_sector_table_entry), 1, target_handle_mds) != 1) {
					fprintf(stderr, "Error writing bad sector table entry!\n");
					throw EXIT_FAILURE;
				}
			}
		}
	}

	protected:

	FILE* target_handle_mds;
	FILE* target_handle_mdf;
};












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
