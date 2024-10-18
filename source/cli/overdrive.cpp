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











namespace mdslegacy {
	#pragma pack(push, 1)

	enum class TrackMode: uint8_t {
		NONE = 0x0,
		UNKNOWN_1 = 0x1,
		DVD = 0x2,
		UNKNOWN_3 = 0x3,
		UNKNOWN_4 = 0x4,
		UNKNOWN_5 = 0x5,
		UNKNOWN_6 = 0x6,
		UNKNOWN_7 = 0x7,
		UNKNOWN_8 = 0x8,
		AUDIO = 0x9,
		MODE1 = 0xA,
		MODE2 = 0xB,
		MODE2_FORM1 = 0xC,
		MODE2_FORM2 = 0xD,
		UNKNOWN_E = 0xE,
		UNKNOWN_F = 0xF
	};

	enum class TrackModeFlags: uint8_t {
		UNKNOWN_0 = 0x0,
		UNKNOWN_1 = 0x1,
		UNKNOWN_2 = 0x2,
		UNKNOWN_3 = 0x3,
		UNKNOWN_4 = 0x4,
		UNKNOWN_5 = 0x5,
		UNKNOWN_6 = 0x6,
		UNKNOWN_7 = 0x7,
		UNKNOWN_8 = 0x8,
		UNKNOWN_9 = 0x9,
		UNKNOWN_A = 0xA,
		UNKNOWN_B = 0xB,
		UNKNOWN_C = 0xC,
		UNKNOWN_D = 0xD,
		UNKNOWN_E = 0xE,
		UNKNOWN_F = 0xF
	};

	enum class EntryTypeATrackNumber: uint8_t {
		UNKNOWN = 0x00,
		FIRST_TRACK = 0xA0,
		LAST_TRACK = 0xA1,
		LEAD_OUT_TRACK = 0xA2
	};

	enum class SubchannelMode: uint8_t {
		NONE = 0x00,
		INTERLEAVED_96 = 0x08
	};

	typedef struct {
		uint8_t identifier[16] = { 'M','E','D','I','A',' ','D','E','S','C','R','I','P','T','O','R' };
		uint8_t major_version = 1;
		uint8_t minor_version = 3;
		uint16_t unknown_a = 0;
		uint16_t unknown_b = 1;
		uint16_t unknown_c = 2;
		uint8_t __a[56] = {};
		uint32_t absolute_offset_to_disc_header = 88;
		uint32_t absolute_offset_to_footer = 0;
	} FormatHeader;

	typedef struct {
		int32_t pregap_correction = -150;
		uint32_t sectors_on_disc = 0;
		uint8_t unknown_a = 1;
		uint8_t unknown_b = 0;
		uint8_t entry_count;
		uint8_t entry_count_type_a;
		uint8_t unknown_c = 1;
		uint8_t unknown_d = 0;
		uint8_t track_count;
		uint8_t unknown_e = 0;
		uint8_t __a[4] = {};
		uint32_t absolute_offset_to_entry_table = 112;
	} DiscHeader;

	typedef struct {
		TrackMode track_mode : 4;
		TrackModeFlags track_mode_flags: 4;
		SubchannelMode subchannel_mode;
		uint8_t flags;
		uint8_t __b[1] = {};
		EntryTypeATrackNumber track_number;
		uint8_t __c[3] = {};
		uint8_t address_p = 0;
		uint8_t address_m = 0;
		uint8_t address_s = 0;
		uint8_t address_f = 0;
		uint8_t __d[68] = {};
	} EntryTypeA;

	typedef struct {
		TrackMode track_mode : 4;
		TrackModeFlags track_mode_flags: 4;
		SubchannelMode subchannel_mode;
		uint8_t flags;
		uint8_t __b[1] = {};
		uint8_t track_number;
		uint8_t __c[3] = {};
		uint8_t address_p = 0;
		uint8_t address_m = 0;
		uint8_t address_s = 0;
		uint8_t address_f = 0;
		uint32_t absolute_offset_to_track_table_entry;
		uint16_t sector_length = 2352; // 2352 or 2448 with subchannels
		uint16_t unknown_a = 2;
		uint8_t __d[16] = {};
		uint32_t first_sector_on_disc;
		uint32_t mdf_byte_offset;
		uint8_t __e[4] = {};
		uint32_t unknown_b = 1;
		uint32_t absolute_offset_to_file_table_header;
		uint8_t __f[24] = {};
	} EntryTypeB;

	typedef struct {
		uint8_t __a[24] = {};
	} TrackTableHeader;

	typedef struct {
		uint32_t pregap_sectors;
		uint32_t length_sectors;
	} TrackTableEntry;

	typedef struct {
		uint32_t absolute_offset_to_file_table_entry;
		uint8_t __a[12] = {};
	} FileTableHeader;

	typedef struct {
		char identifier[6] = "*.mdf";
	} FileTableEntry;

	typedef struct {
		uint32_t unknown_a = 1;
		uint32_t absolute_offset_to_bad_sectors_table_header;
	} Footer;

	typedef struct {
		uint32_t unknown_a = 2;
		uint32_t unknown_b = 4;
		uint32_t unknown_c = 1;
		uint32_t bad_sector_count;
	} BadSectorTableHeader;

	typedef struct {
		uint32_t bad_sector_number;
	} BadSectorTableEntry;

	#pragma pack(pop)
}



























class MDSImageFormat {
	public:

	MDSImageFormat(const std::string& directory, const std::string& filename) {
		auto target_path_mds = directory + filename + ".mds";
		auto target_handle_mds = fopen(target_path_mds.c_str(), "wb+");
		if (target_handle_mds == nullptr) {
			fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_mds.c_str());
			throw EXIT_FAILURE;
		}
		auto target_path_mdf = directory + filename + ".mdf";
		auto target_handle_mdf = fopen(target_path_mdf.c_str(), "wb+");
		if (target_handle_mdf == nullptr) {
			fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_mdf.c_str());
			throw EXIT_FAILURE;
		}
		this->target_handle_mds = target_handle_mds;
		this->target_handle_mdf = target_handle_mdf;
	}

	~MDSImageFormat() {
		fclose(this->target_handle_mds);
		fclose(this->target_handle_mdf);
	}

	auto write_sector_data(const cdb::ReadTOCResponseNormalTOCEntry& track, const uint8_t* data, size_t size) -> bool {
		(void)track;
		auto bytes_expected = size;
		auto bytes_returned = fwrite(data, 1, bytes_expected, this->target_handle_mdf);
		return bytes_returned == bytes_expected;
	}

	auto write_subchannel_data(const cdb::ReadTOCResponseNormalTOCEntry& track, const uint8_t* data) -> bool {
		(void)track;
		auto bytes_expected = (size_t)cd::SUBCHANNELS_LENGTH;
		auto bytes_returned = fwrite(data, 1, bytes_expected, this->target_handle_mdf);
		return bytes_returned == bytes_expected;
	}

	auto write_index(const drive::Drive& drive, const cdb::ReadTOCResponseNormalTOC& toc, const cdb::ReadTOCResponseFullTOC& toc_ex, bool subchannels, const std::vector<int>& bad_sector_numbers, const std::vector<unsigned int>& track_pregap_sectors_list, const std::vector<unsigned int>& track_length_sectors_list) -> void {
		auto &first_track = toc.entries[toc.header.first_track_or_session_number - 1];
		auto &lead_out_track = toc.entries[toc.header.last_track_or_session_number + 1 - 1];
		auto track_count = toc.header.last_track_or_session_number - toc.header.first_track_or_session_number + 1;
		auto sector_count = cd::get_sector_from_address(lead_out_track.track_start_address) - cd::get_sector_from_address(first_track.track_start_address);
		auto absolute_offset_to_track_table_entry = sizeof(mdslegacy::FormatHeader) + sizeof(mdslegacy::DiscHeader) + 3 * sizeof(mdslegacy::EntryTypeA) + track_count * sizeof(mdslegacy::EntryTypeB) + sizeof(mdslegacy::TrackTableHeader);
		auto absolute_offset_to_file_table_header = absolute_offset_to_track_table_entry + track_count * sizeof(mdslegacy::TrackTableEntry);
		auto absolute_offset_to_file_table_entry = absolute_offset_to_file_table_header + sizeof(mdslegacy::FileTableHeader);
		auto absolute_offset_to_footer = absolute_offset_to_file_table_entry + sizeof(mdslegacy::FileTableEntry);
		auto absolute_offset_to_bad_sectors_table_header = absolute_offset_to_footer + sizeof(mdslegacy::Footer);
		auto format_header = mdslegacy::FormatHeader();
		format_header.absolute_offset_to_footer = bad_sector_numbers.size() == 0 ? 0 : absolute_offset_to_footer;
		if (fwrite(&format_header, sizeof(format_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing format header!\n");
			throw EXIT_FAILURE;
		}
		auto disc_header = mdslegacy::DiscHeader();
		disc_header.sectors_on_disc = sector_count;
		disc_header.entry_count = track_count + 3;
		disc_header.entry_count_type_a = 3;
		disc_header.track_count = track_count;
		if (fwrite(&disc_header, sizeof(disc_header), 1, target_handle_mds) != 1) {
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
				auto current_track_entry = mdslegacy::EntryTypeA();
				current_track_entry.track_mode = mdslegacy::TrackMode::NONE;
				current_track_entry.track_mode_flags = mdslegacy::TrackModeFlags::UNKNOWN_0;
				std::memcpy((UCHAR*)&current_track_entry + offsetof(mdslegacy::EntryTypeA, subchannel_mode), &current_track, sizeof(current_track));
				current_track_entry.subchannel_mode = subchannels ? mdslegacy::SubchannelMode::INTERLEAVED_96 : mdslegacy::SubchannelMode::NONE;
				if (fwrite(&current_track_entry, sizeof(current_track_entry), 1, target_handle_mds) != 1) {
					fprintf(stderr, "Error writing current track entry!\n");
					throw EXIT_FAILURE;
				}
			} else {
				auto current_track_entry = mdslegacy::EntryTypeB();
				auto current_track_type = drive.determine_track_type(toc_ex, toc_ex_track_index);
				auto current_track_mode = this->get_track_mode(current_track_type);
				current_track_entry.track_mode = current_track_mode;
				current_track_entry.track_mode_flags = current_track_mode == mdslegacy::TrackMode::MODE2_FORM1 || current_track_mode == mdslegacy::TrackMode::MODE2_FORM2 ? mdslegacy::TrackModeFlags::UNKNOWN_E : mdslegacy::TrackModeFlags::UNKNOWN_A;
				std::memcpy((UCHAR*)&current_track_entry + offsetof(mdslegacy::EntryTypeA, subchannel_mode), &current_track, sizeof(current_track));
				current_track_entry.subchannel_mode = subchannels ? mdslegacy::SubchannelMode::INTERLEAVED_96 : mdslegacy::SubchannelMode::NONE;
				current_track_entry.sector_length = subchannels ? cd::SECTOR_LENGTH + cd::SUBCHANNELS_LENGTH : cd::SECTOR_LENGTH;
				current_track_entry.first_sector_on_disc = first_sector_on_disc;
				current_track_entry.mdf_byte_offset = mdf_byte_offset;
				current_track_entry.absolute_offset_to_track_table_entry = absolute_offset_to_track_table_entry + (track_number - toc.header.first_track_or_session_number) * sizeof(mdslegacy::TrackTableEntry);
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
		auto track_table_header = mdslegacy::TrackTableHeader();
		if (fwrite(&track_table_header, sizeof(track_table_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing track table header!\n");
			throw EXIT_FAILURE;
		}
		for (auto i = toc.header.first_track_or_session_number; i <= toc.header.last_track_or_session_number; i += 1) {
			auto track_table_entry = mdslegacy::TrackTableEntry();
			track_table_entry.pregap_sectors = track_pregap_sectors_list.at(i - toc.header.first_track_or_session_number);
			track_table_entry.length_sectors = track_length_sectors_list.at(i - toc.header.first_track_or_session_number);
			if (fwrite(&track_table_entry, sizeof(track_table_entry), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing track table entry!\n");
				throw EXIT_FAILURE;
			}
		}
		auto file_table_header = mdslegacy::FileTableHeader();
		file_table_header.absolute_offset_to_file_table_entry = absolute_offset_to_file_table_entry;
		if (fwrite(&file_table_header, sizeof(file_table_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing file table header!\n");
			throw EXIT_FAILURE;
		}
		auto file_table_entry = mdslegacy::FileTableEntry();
		if (fwrite(&file_table_entry, sizeof(file_table_entry), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing file table entry!\n");
			throw EXIT_FAILURE;
		}
		if (bad_sector_numbers.size() > 0) {
			auto footer = mdslegacy::Footer();
			footer.absolute_offset_to_bad_sectors_table_header = absolute_offset_to_bad_sectors_table_header;
			if (fwrite(&footer, sizeof(footer), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing footer!\n");
				throw EXIT_FAILURE;
			}
			auto bad_sector_table_header = mdslegacy::BadSectorTableHeader();
			bad_sector_table_header.bad_sector_count = bad_sector_numbers.size();
			if (fwrite(&bad_sector_table_header, sizeof(bad_sector_table_header), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing bad sector table header!\n");
				throw EXIT_FAILURE;
			}
			for (auto bad_sector_number : bad_sector_numbers) {
				auto bad_sector_table_entry = mdslegacy::BadSectorTableEntry();
				bad_sector_table_entry.bad_sector_number = bad_sector_number;
				if (fwrite(&bad_sector_table_entry, sizeof(bad_sector_table_entry), 1, target_handle_mds) != 1) {
					fprintf(stderr, "Error writing bad sector table entry!\n");
					throw EXIT_FAILURE;
				}
			}
		}
	}

	protected:

	auto get_track_mode(disc::TrackType track_type) -> mdslegacy::TrackMode {
		if (track_type == disc::TrackType::AUDIO_2_CHANNELS) {
			return mdslegacy::TrackMode::AUDIO;
		}
		if (track_type == disc::TrackType::AUDIO_4_CHANNELS) {
			return mdslegacy::TrackMode::AUDIO;
		}
		if (track_type == disc::TrackType::DATA_MODE0) {
			return mdslegacy::TrackMode::NONE;
		}
		if (track_type == disc::TrackType::DATA_MODE1) {
			return mdslegacy::TrackMode::MODE1;
		}
		if (track_type == disc::TrackType::DATA_MODE2) {
			return mdslegacy::TrackMode::MODE2;
		}
		if (track_type == disc::TrackType::DATA_MODE2_FORM1) {
			return mdslegacy::TrackMode::MODE2_FORM1;
		}
		if (track_type == disc::TrackType::DATA_MODE2_FORM2) {
			return mdslegacy::TrackMode::MODE2_FORM2;
		}
		throw EXIT_FAILURE;
	}

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
