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
#include "lib.h"

using namespace overdrive;
using namespace type;

#define APP_NAME "Overdrive"
#define APP_VERSION "0.0.0"

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
) -> void {
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
	if (sptd_sense.sense[0] == (byte_t)sense::ResponseCodes::FIXED_CURRENT) {
		auto& sense = *reinterpret_cast<sense::FixedFormat*>(sptd_sense.sense);
		fprintf(stderr, "[WARNING] Sense info 0x%.1X 0x%.2X 0x%.2X!\n", (unsigned)sense.sense_key, sense.additional_sense_code, sense.additional_sense_code_qualifier);
	}
}

typedef struct {
	unsigned char data[cd::SECTOR_LENGTH];
	unsigned int counter;
} ExtractedCDDASector;

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

enum class FileFormat {
	MDF_MDS,
	BIN_CUE
};

namespace mds {
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

class ImageFormat {
	public:

	virtual ~ImageFormat() {}

	virtual auto write_sector_data(const cdb::ReadTOCResponseNormalTOCEntry& track, const uint8_t* data, size_t size) -> bool = 0;
	virtual auto write_subchannel_data(const cdb::ReadTOCResponseNormalTOCEntry& track, const uint8_t* data) -> bool = 0;
	virtual auto write_index(const drive::Drive& drive, const cdb::ReadTOCResponseNormalTOC& toc, const cdb::ReadTOCResponseFullTOC& toc_ex, bool subchannels, const std::vector<int>& bad_sector_numbers, const std::vector<unsigned int>& track_pregap_sectors_list, const std::vector<unsigned int>& track_length_sectors_list) -> void = 0;

	protected:
};



























class MDSImageFormat: ImageFormat {
	public:

	MDSImageFormat(const std::string& directory, const std::string& filename): ImageFormat() {
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
		auto absolute_offset_to_track_table_entry = sizeof(mds::FormatHeader) + sizeof(mds::DiscHeader) + 3 * sizeof(mds::EntryTypeA) + track_count * sizeof(mds::EntryTypeB) + sizeof(mds::TrackTableHeader);
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
		auto disc_header = mds::DiscHeader();
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
				auto current_track_mode = this->get_track_mode(current_track_type);
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
				bad_sector_table_entry.bad_sector_number = bad_sector_number;
				if (fwrite(&bad_sector_table_entry, sizeof(bad_sector_table_entry), 1, target_handle_mds) != 1) {
					fprintf(stderr, "Error writing bad sector table entry!\n");
					throw EXIT_FAILURE;
				}
			}
		}
	}

	protected:

	auto get_track_mode(drive::TrackType track_type) -> mds::TrackMode {
		if (track_type == drive::TrackType::AUDIO_2_CHANNELS) {
			return mds::TrackMode::AUDIO;
		}
		if (track_type == drive::TrackType::AUDIO_4_CHANNELS) {
			return mds::TrackMode::AUDIO;
		}
		if (track_type == drive::TrackType::DATA_MODE0) {
			return mds::TrackMode::NONE;
		}
		if (track_type == drive::TrackType::DATA_MODE1) {
			return mds::TrackMode::MODE1;
		}
		if (track_type == drive::TrackType::DATA_MODE2) {
			return mds::TrackMode::MODE2;
		}
		if (track_type == drive::TrackType::DATA_MODE2_FORM1) {
			return mds::TrackMode::MODE2_FORM1;
		}
		if (track_type == drive::TrackType::DATA_MODE2_FORM2) {
			return mds::TrackMode::MODE2_FORM2;
		}
		throw EXIT_FAILURE;
	}

	FILE* target_handle_mds;
	FILE* target_handle_mdf;
};





















class BINCUEImageFormat: ImageFormat {
	public:

	BINCUEImageFormat(const std::string& directory, const std::string& filename, bool split_tracks, bool add_wave_headers, bool complete_data_sectors): ImageFormat() {
		auto target_path_cue = directory + filename + ".cue";
		auto target_handle_cue = fopen(target_path_cue.c_str(), "wb+");
		if (target_handle_cue == nullptr) {
			fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_cue.c_str());
			throw EXIT_FAILURE;
		}
		this->split_tracks = split_tracks;
		this->add_wave_headers = add_wave_headers;
		this->complete_data_sectors = complete_data_sectors;
		this->directory = directory;
		this->filename = filename;
		this->target_handle_cue = target_handle_cue;
		this->target_handle_bin = nullptr;
		this->current_track = cdb::ReadTOCResponseNormalTOCEntry();
	}

	~BINCUEImageFormat() {
		this->update_wave_header();
		fclose(this->target_handle_cue);
		fclose(this->target_handle_bin);
	}

	auto write_sector_data(const cdb::ReadTOCResponseNormalTOCEntry& track, const uint8_t* data, size_t size) -> bool {
		auto bytes_expected = size;
		auto bytes_returned = fwrite(data, 1, bytes_expected, this->get_track_handle(track));
		return bytes_returned == bytes_expected;
	}

	auto write_subchannel_data(const cdb::ReadTOCResponseNormalTOCEntry& track, const uint8_t* data) -> bool {
		fprintf(stderr, "Subchannel data cannot be stored using the BIN/CUE format!\n");
		throw EXIT_FAILURE;
		auto bytes_expected = (size_t)cd::SUBCHANNELS_LENGTH;
		auto bytes_returned = fwrite(data, 1, bytes_expected, this->get_track_handle(track));
		return bytes_returned == bytes_expected;
	}

	auto write_index(const drive::Drive& drive, const cdb::ReadTOCResponseNormalTOC& toc, const cdb::ReadTOCResponseFullTOC& toc_ex, bool subchannels, const std::vector<int>& bad_sector_numbers, const std::vector<unsigned int>& track_pregap_sectors_list, const std::vector<unsigned int>& track_length_sectors_list) -> void {
		(void)drive;
		(void)toc_ex;
		(void)subchannels;
		(void)bad_sector_numbers;
		if (this->split_tracks) {
			auto offset = 0;
			for (auto track_number = toc.header.first_track_or_session_number; track_number <= toc.header.last_track_or_session_number; track_number += 1) {
				auto &current_track = toc.entries[track_number - 1];
				auto current_track_type = cd::get_track_category(current_track.control);
				auto extension = this->add_wave_headers && cd::is_audio_category(current_track_type) ? ".wav" : ".bin";
				auto tag = this->add_wave_headers && cd::is_audio_category(current_track_type) ? "WAVE" : "BINARY";
				fprintf(this->target_handle_cue, "FILE \"%s_%.2i%s\" %s\n", this->filename.c_str(), track_number, extension, tag);
				fprintf(this->target_handle_cue, "\tTRACK %.2i %s\n", track_number, cd::is_audio_category(current_track_type) ? "AUDIO" : this->complete_data_sectors ? "MODE1/2352" : "MODE1/2048");
				auto track_pregap_sectors = track_pregap_sectors_list.at(track_number - 1);
				auto track_pregap_sectors_to_write = track_number == toc.header.first_track_or_session_number ? 0 : track_pregap_sectors;
				auto pregap_address = cd::get_address_from_sector(track_pregap_sectors_to_write);
				fprintf(this->target_handle_cue, "\t\tPREGAP %.2i:%.2i:%.2i\n", pregap_address.m, pregap_address.s, pregap_address.f);
				auto offset_address = cd::get_address_from_sector(offset);
				fprintf(this->target_handle_cue, "\t\tINDEX %.2i %.2i:%.2i:%.2i\n", 1, offset_address.m, offset_address.s, offset_address.f);
			}
		} else {
			fprintf(this->target_handle_cue, "FILE \"%s\" BINARY\n", this->filename.c_str());
			auto offset = 0;
			for (auto track_number = toc.header.first_track_or_session_number; track_number <= toc.header.last_track_or_session_number; track_number += 1) {
				auto &current_track = toc.entries[track_number - 1];
				auto current_track_type = cd::get_track_category(current_track.control);
				fprintf(this->target_handle_cue, "\tTRACK %.2i %s\n", track_number, cd::is_audio_category(current_track_type) ? "AUDIO" : this->complete_data_sectors ? "MODE1/2352" : "MODE1/2048");
				auto track_pregap_sectors = track_pregap_sectors_list.at(track_number - 1);
				auto track_length_sectors = track_length_sectors_list.at(track_number - 1);
				auto track_pregap_sectors_to_write = track_number == toc.header.first_track_or_session_number ? 0 : track_pregap_sectors;
				auto pregap_address = cd::get_address_from_sector(track_pregap_sectors_to_write);
				fprintf(this->target_handle_cue, "\t\tPREGAP %.2i:%.2i:%.2i\n", pregap_address.m, pregap_address.s, pregap_address.f);
				auto offset_address = cd::get_address_from_sector(offset);
				fprintf(this->target_handle_cue, "\t\tINDEX %.2i %.2i:%.2i:%.2i\n", 1, offset_address.m, offset_address.s, offset_address.f);
				offset += track_length_sectors;
			}
		}
	}

	protected:

	auto update_wave_header() -> void {
		if (this->target_handle_bin != nullptr && this->add_wave_headers && cd::is_audio_category(cd::get_track_category(this->current_track.control))) {
			auto header = wav::Header();
			auto file_size = ftell(this->target_handle_bin);
			fseek(this->target_handle_bin, 0, SEEK_SET);
			if (fread(&header, sizeof(header), 1, this->target_handle_bin) != 1) {
				fprintf(stderr, "Failed reading WAV header from file!\n");
				throw EXIT_FAILURE;
			}
			fseek(this->target_handle_bin, 0, SEEK_SET);
			header.riff_length = file_size - 8;
			header.data_length = file_size - sizeof(header);
			if (fwrite(&header, sizeof(header), 1, this->target_handle_bin) != 1) {
				fprintf(stderr, "Failed writing WAV header to file!\n");
				throw EXIT_FAILURE;
			}
			fseek(this->target_handle_bin, file_size, SEEK_SET);
		}
	}

	auto get_track_handle(const cdb::ReadTOCResponseNormalTOCEntry& track) -> FILE* {
		if (this->split_tracks) {
			if (this->current_track.track_number != track.track_number) {
				this->update_wave_header();
				fclose(this->target_handle_bin);
				this->target_handle_bin = nullptr;
			}
			if (this->target_handle_bin != nullptr) {
				return this->target_handle_bin;
			} else {
				auto track_type = cd::get_track_category(track.control);
				char buffer[3] = {};
				snprintf(buffer, sizeof(buffer), "%.2u", track.track_number);
				auto extension = this->add_wave_headers && cd::is_audio_category(track_type) ? ".wav" : ".bin";
				auto target_path_bin = this->directory + this->filename + "_" + buffer + extension;
				auto target_handle_bin = fopen(target_path_bin.c_str(), "wb+");
				if (target_handle_bin == nullptr) {
					fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_bin.c_str());
					throw EXIT_FAILURE;
				}
				if (this->add_wave_headers && cd::is_audio_category(track_type)) {
					auto wave_header = wav::Header();
					auto bytes_expected = sizeof(wave_header);
					auto bytes_returned = fwrite(&wave_header, 1, bytes_expected, target_handle_bin);
					if (bytes_returned != bytes_expected) {
						fprintf(stderr, "Failed writing WAV header to file \"%s\"!\n", target_path_bin.c_str());
						throw EXIT_FAILURE;
					}
				}
				this->target_handle_bin = target_handle_bin;
				this->current_track = track;
				return this->target_handle_bin;
			}
		} else {
			auto handle = this->target_handle_bin;
			if (handle == nullptr) {
				auto target_path_bin = this->directory + this->filename + ".bin";
				handle = fopen(target_path_bin.c_str(), "wb+");
				if (handle == nullptr) {
					fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_bin.c_str());
					throw EXIT_FAILURE;
				}
				this->target_handle_bin = handle;
			}
			return handle;
		}
	}

	bool split_tracks;
	bool add_wave_headers;
	bool complete_data_sectors;
	std::string directory;
	std::string filename;
	FILE* target_handle_cue;
	FILE* target_handle_bin;
	cdb::ReadTOCResponseNormalTOCEntry current_track;
};

auto get_image_format(FileFormat format, const std::string& directory, const std::string& filename, bool split_tracks, bool add_wave_headers, bool complete_data_sectors)
-> std::shared_ptr<ImageFormat> {
	if (format == FileFormat::MDF_MDS) {
		return std::shared_ptr<ImageFormat>((ImageFormat*)new MDSImageFormat(directory, filename));
	}
	return std::shared_ptr<ImageFormat>((ImageFormat*)new BINCUEImageFormat(std::string(directory), std::string(filename), split_tracks, add_wave_headers, complete_data_sectors));
}






















auto save(
	si_t argc,
	ch08_t** argv
) -> void {
	auto subchannels = false;
	auto directory = std::string(".\\private\\");
	auto filename = std::string("image");
	auto format = FileFormat::MDF_MDS;
	auto drive_argument = std::optional<std::string>();
	auto max_read_retries = 8;
	auto read_offset_correction = std::optional<int>();
	auto max_audio_read_passes = 8;
	auto split_tracks = false;
	auto add_wave_headers = false;
	auto complete_data_sectors = true;
	auto unrecognized_arguments = std::vector<char*>();
	auto positional_index = 0;
	for (auto i = 2; i < argc; i += 1) {
		auto argument = argv[i];
		if (false) {
		} else if (strstr(argument, "--subchannels=") == argument) {
			auto value = argument + sizeof("--subchannels=") - 1;
			if (false) {
			} else if (strcmp(value, "false") == 0) {
				subchannels = false;
			} else if (strcmp(value, "true") == 0) {
				subchannels = true;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--directory=") == argument) {
			auto value = argument + sizeof("--directory=") - 1;
			directory = value;
		} else if (strstr(argument, "--filename=") == argument) {
			auto value = argument + sizeof("--filename=") - 1;
			auto extension = strstr(value, ".");
			if (extension != nullptr) {
				if (strstr(extension, ".mds") == extension || strstr(extension, ".mdf") == extension) {
					format = FileFormat::MDF_MDS;
				} else if (strstr(extension, ".cue") == extension || strstr(extension, ".bin") == extension) {
					format = FileFormat::BIN_CUE;
				}
				filename = std::string(value, extension - value);
			} else {
				filename = value;
			}
		} else if (strstr(argument, "--format=") == argument) {
			auto value = argument + sizeof("--format=") - 1;
			if (false) {
			} else if (strcmp(value, "MDF/MDS") == 0) {
				format = FileFormat::MDF_MDS;
			} else if (strcmp(value, "BIN/CUE") == 0) {
				format = FileFormat::BIN_CUE;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--drive=") == argument) {
			auto value = argument + sizeof("--drive=") - 1;
			auto value_length = strlen(value);
			if (false) {
			} else if (value_length == 1 && value[0] > 'A' && value[0] < 'Z') {
				drive_argument = std::string(value, 1);
			} else if (value_length == 2 && value[1] == ':') {
				drive_argument = std::string(value, 1);
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--max-read-retries=") == argument) {
			auto value = argument + sizeof("--max-read-retries=") - 1;
			auto parsed_value = atoi(value);
			if (false) {
			} else if (parsed_value >= 0 && parsed_value <= 255) {
				max_read_retries = parsed_value;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--read-offset-correction=") == argument) {
			auto value = argument + sizeof("--read-offset-correction=") - 1;
			auto parsed_value = atoi(value);
			if (false) {
			} else if (parsed_value >= 0 - int(10 * cd::SECTOR_LENGTH) && parsed_value <= 0 + int(10 * cd::SECTOR_LENGTH)) {
				read_offset_correction = parsed_value;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--max-audio-read-passes=") == argument) {
			auto value = argument + sizeof("--max-audio-read-passes=") - 1;
			auto parsed_value = atoi(value);
			if (false) {
			} else if (parsed_value >= 1 && parsed_value <= 16) {
				max_audio_read_passes = parsed_value;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--split-tracks=") == argument) {
			auto value = argument + sizeof("--split-tracks=") - 1;
			if (false) {
			} else if (strcmp(value, "false") == 0) {
				split_tracks = false;
			} else if (strcmp(value, "true") == 0) {
				split_tracks = true;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--add-wave-headers=") == argument) {
			auto value = argument + sizeof("--add-wave-headers=") - 1;
			if (false) {
			} else if (strcmp(value, "false") == 0) {
				add_wave_headers = false;
			} else if (strcmp(value, "true") == 0) {
				add_wave_headers = true;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (strstr(argument, "--complete-data-sectors=") == argument) {
			auto value = argument + sizeof("--complete-data-sectors=") - 1;
			if (false) {
			} else if (strcmp(value, "false") == 0) {
				complete_data_sectors = false;
			} else if (strcmp(value, "true") == 0) {
				complete_data_sectors = true;
			} else {
				unrecognized_arguments.push_back(argument);
			}
		} else if (positional_index == 0) {
			auto value = argument;
			auto value_length = strlen(value);
			if (false) {
			} else if (value_length == 1 && value[0] > 'A' && value[0] < 'Z') {
				drive_argument = std::string(value, 1);
			} else if (value_length == 2 && value[1] == ':') {
				drive_argument = std::string(value, 1);
			} else {
				unrecognized_arguments.push_back(argument);
			}
			positional_index += 1;
		} else {
			unrecognized_arguments.push_back(argument);
		}
	}
	auto missing_arguments = std::vector<std::string>();
	if (!drive_argument) {
		missing_arguments.push_back("drive");
	}
	if (unrecognized_arguments.size() > 0 || missing_arguments.size() > 0) {
		fprintf(stderr, "%s v%s\n", APP_NAME, APP_VERSION);
		fprintf(stderr, "\n");
		for (auto &unrecognized_argument : unrecognized_arguments) {
			fprintf(stderr, "Unrecognized argument \"%s\"!\n", unrecognized_argument);
		}
		for (auto &missing_argument : missing_arguments) {
			fprintf(stderr, "Missing argument \"%s\"!\n", missing_argument.c_str());
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "Arguments:\n");
		fprintf(stderr, "\t--subchannels=boolean\n");
		fprintf(stderr, "\t\tSet subchannel extraction (false by default).\n");
		fprintf(stderr, "\t--directory=string\n");
		fprintf(stderr, "\t\tSet target directory (current working directory by default).\n");
		fprintf(stderr, "\t--filename=string\n");
		fprintf(stderr, "\t\tSet target filename (\"image\" by default).\n");
		fprintf(stderr, "\t--format=enum\n");
		fprintf(stderr, "\t\tSet target format (MDF/MDS by default).\n");
		fprintf(stderr, "\t\t\tMDF/MDS.\n");
		fprintf(stderr, "\t\t\tBIN/CUE.\n");
		fprintf(stderr, "\t--drive=string\n");
		fprintf(stderr, "\t\tSet optical drive letter.\n");
		fprintf(stderr, "\t--max-read-retries=integer[0,255]\n");
		fprintf(stderr, "\t\tSet max read retries made before producing a read error (8 by default).\n");
		fprintf(stderr, "\t--read-offset-correction=integer[%llu,%llu]\n", 0 - (cd::SECTOR_LENGTH * 10), 0 + (cd::SECTOR_LENGTH * 10));
		fprintf(stderr, "\t\tSet read offset correction (samples) for audio track extraction (0 by default).\n");
		fprintf(stderr, "\t--max-audio-read-passes=integer[1,16]\n");
		fprintf(stderr, "\t\tSet maximum number of audio read passes made (8 by default).\n");
		fprintf(stderr, "\t--split-tracks=boolean\n");
		fprintf(stderr, "\t\tStore tracks as separate files when format is BIN/CUE (false by default).\n");
		fprintf(stderr, "\t--add-wave-headers=boolean\n");
		fprintf(stderr, "\t\tAdd wave headers to audio tracks when format is BIN/CUE (false by default).\n");
		fprintf(stderr, "\t--complete-data-sectors=boolean\n");
		fprintf(stderr, "\t\tStore complete sectors for data tracks (true by default).\n");
		throw EXIT_FAILURE;
	} else {
		auto handle = get_handle(drive_argument.value());
		auto scsi_drive = drive::create_drive(handle, pass_through_direct);
		auto standard_inquiry = scsi_drive.read_standard_inquiry();
		auto vendor = std::string(standard_inquiry.vendor_identification, sizeof(standard_inquiry.vendor_identification));
		auto product = std::string(standard_inquiry.product_identification, sizeof(standard_inquiry.product_identification));
		fprintf(stderr, "Vendor is \"%s\"\n", string::trim(vendor).c_str());
		fprintf(stderr, "Product is \"%s\"\n", string::trim(product).c_str());
		auto accuraterip_database = accuraterip::Database();
		auto optional_rac = accuraterip_database.get_read_offset_correction_value(standard_inquiry.vendor_identification, standard_inquiry.product_identification);
		if (optional_rac) {
			auto rac = *optional_rac;
			fprintf(stderr, "Detected read offset correction as %i samples (%llu bytes)\n", rac, (rac * cdda::STEREO_SAMPLE_LENGTH));
			if (!read_offset_correction) {
				read_offset_correction = rac;
			}
		}
		auto toc = scsi_drive.read_toc();
		cdb::validate_toc(toc);
		auto toc_ex = scsi_drive.read_full_toc();
		auto subchannel_offset = scsi_drive.get_subchannels_data_offset();
		fprintf(stderr, "Subchannel data offset is %llu\n", subchannel_offset);
		auto c2_offset = scsi_drive.get_c2_data_offset();
		fprintf(stderr, "C2 data offset is %llu\n", c2_offset);
		{
			auto error_recovery_mode_page = scsi_drive.read_error_recovery_mode_page();
			error_recovery_mode_page.page.read_retry_count = max_read_retries;
			scsi_drive.write_error_recovery_mode_page(error_recovery_mode_page);
		}
		auto session_type = cdb::get_session_type(toc_ex);
		if (session_type == cdb::SessionType::CDDA_OR_CDROM) {
			fprintf(stderr, "Disc contains a CDDA or CDROM session\n");
		} else if (session_type == cdb::SessionType::CDI) {
			fprintf(stderr, "Disc contains a CDI session\n");
		} else if (session_type == cdb::SessionType::CDXA_OR_DDCD) {
			fprintf(stderr, "Disc contains a CDXA or DDCD session\n");
		}
		auto data_offset = size_t(0);
		auto data_length = size_t(cd::SECTOR_LENGTH);
		if (!complete_data_sectors) {
			data_length = cdrom::MODE1_DATA_LENGTH;
			if (session_type == cdb::SessionType::CDDA_OR_CDROM) {
				data_offset = offsetof(cdrom::Mode1Sector, user_data);
			} else if (session_type == cdb::SessionType::CDI) {
				throw EXIT_FAILURE;
			} else if (session_type == cdb::SessionType::CDXA_OR_DDCD) {
				data_offset = offsetof(cdxa::Mode2Form1Sector, user_data);
			}
		}
		fprintf(stderr, "Data track sector offset is %llu\n", data_offset);
		fprintf(stderr, "Data track sector length is %llu\n", data_length);
		{
			auto mode_sense = scsi_drive.read_capabilites_and_mechanical_status_page();
			fprintf(stderr, "Drive has a read cache size of %u kB\n", byteswap::byteswap16(mode_sense.page.buffer_size_supported_be));
			fprintf(stderr, "Drive %s read audio streams accurately\n", mode_sense.page.cdda_stream_is_accurate ? "can" : "cannot");
			fprintf(stderr, "Drive %s support for reading C2 error pointers\n", mode_sense.page.c2_pointers_supported ? "has" : "lacks");
			if (!mode_sense.page.cdda_stream_is_accurate) {
				throw EXIT_FAILURE;
			}
		}
		auto image_format = get_image_format(format, directory, filename, split_tracks, add_wave_headers, complete_data_sectors);
		auto &first_track = toc.entries[toc.header.first_track_or_session_number - 1];
		auto &lead_out_track = toc.entries[toc.header.last_track_or_session_number + 1 - 1];
		auto track_count = toc.header.last_track_or_session_number - toc.header.first_track_or_session_number + 1;
		auto sector_count = cd::get_sector_from_address(lead_out_track.track_start_address) - cd::get_sector_from_address(first_track.track_start_address);
		fprintf(stderr, "Disc contains %i tracks\n", track_count);
		fprintf(stderr, "Disc length is %u sectors\n", sector_count);
		auto read_offset_correction_value = read_offset_correction.value_or(0);
		auto read_offset_correction_bytes = read_offset_correction_value * cdda::STEREO_SAMPLE_LENGTH;
		fprintf(stderr, "Read offset correction is set to %i samples (%llu bytes)\n", read_offset_correction_value, read_offset_correction_bytes);
		auto track_pregap_sectors_list = std::vector<unsigned int>();
		track_pregap_sectors_list.push_back(2 * cd::SECTORS_PER_SECOND);
		for (auto i = toc.header.first_track_or_session_number + 1; i <= toc.header.last_track_or_session_number; i += 1) {
			auto &last_track = toc.entries[i - 1 - 1];
			auto &current_track = toc.entries[i - 1];
			auto last_track_type = cd::get_track_category(last_track.control);
			auto current_track_type = cd::get_track_category(current_track.control);
			auto track_type_change = current_track_type != last_track_type;
			auto track_pregap_seconds = track_type_change ? 2 : 0;
			auto track_pregap_sectors = track_pregap_seconds * cd::SECTORS_PER_SECOND;
			track_pregap_sectors_list.push_back(track_pregap_sectors);
		}
		auto track_length_sectors_list = std::vector<unsigned int>();
		auto bad_sector_numbers = std::vector<int>();
		auto empty_cd_sector = cdb::ReadCDResponseDataA();
		auto cd_sector = cdb::ReadCDResponseDataA();
		auto start_ms = get_timestamp_ms();
		for (auto i = toc.header.first_track_or_session_number; i <= toc.header.last_track_or_session_number; i += 1) {
			fprintf(stderr, "Processing track %u\n", i);
			auto &current_track = toc.entries[i - 1];
			auto &next_track = toc.entries[i + 1 - 1];
			auto current_track_lba = cd::get_sector_from_address(current_track.track_start_address) - 150;
			auto next_track_lba = cd::get_sector_from_address(next_track.track_start_address) - 150;
			fprintf(stderr, "Current track starts at sector %u\n", current_track_lba);
			fprintf(stderr, "Next track starts at sector %u\n", next_track_lba);
			auto next_track_pregap_sectors = i == toc.header.last_track_or_session_number ? 0 : track_pregap_sectors_list.at(i + 1 - toc.header.first_track_or_session_number);
			fprintf(stderr, "Next track has a pregap of %i sectors\n", next_track_pregap_sectors);
			auto first_sector = current_track_lba;
			auto last_sector = next_track_lba - next_track_pregap_sectors;
			auto track_length_sectors = last_sector - first_sector;
			fprintf(stderr, "Track length is %u sectors\n", track_length_sectors);
			track_length_sectors_list.push_back(track_length_sectors);
			auto current_track_type = cd::get_track_category(current_track.control);
			if (cd::is_audio_category(current_track_type)) {
				fprintf(stderr, "Current track contains audio\n");
				auto start_offset_bytes = (first_sector * cd::SECTOR_LENGTH) + read_offset_correction_bytes;
				auto end_offset_bytes = (last_sector * cd::SECTOR_LENGTH) + read_offset_correction_bytes;
				auto adjusted_first_sector = idiv::floor(start_offset_bytes, cd::SECTOR_LENGTH);
				auto adjusted_last_sector = idiv::ceil(end_offset_bytes, cd::SECTOR_LENGTH);
				auto adjusted_track_length_sectors = adjusted_last_sector - adjusted_first_sector;
				fprintf(stderr, "Extracting %i sectors from %i to %i\n", adjusted_track_length_sectors, adjusted_first_sector, adjusted_last_sector - 1);
				auto track_data = std::vector<uint8_t>(adjusted_track_length_sectors * cd::SECTOR_LENGTH);
				auto track_data_start_offset = read_offset_correction_bytes - ((adjusted_first_sector - first_sector) * cd::SECTOR_LENGTH);
				fprintf(stderr, "The first %llu bytes will be discarded\n", track_data_start_offset);
				auto extracted_cdda_sectors_list = std::vector<std::vector<ExtractedCDDASector>>(track_length_sectors);
				for (auto audio_pass_index = 0; audio_pass_index < max_audio_read_passes; audio_pass_index += 1) {
					fprintf(stderr, "Running audio extraction pass %i\n", audio_pass_index);
					auto c2_errors = false;
					for (auto sector_index = adjusted_first_sector; sector_index < adjusted_last_sector; sector_index += 1) {
						try {
							auto c2_data = (UCHAR*)&cd_sector + c2_offset;
							scsi_drive.read_sector(sector_index, &cd_sector.sector_data, &cd_sector.subchannels_data, &cd_sector.c2_data);
							for (auto i = 0; i < (int)sizeof(cd_sector.c2_data); i++) {
								if (c2_data[i] != 0) {
									c2_errors = true;
									break;
								}
							}
							auto target = track_data.data() + (sector_index - adjusted_first_sector) * cd::SECTOR_LENGTH;
							std::memcpy(target, cd_sector.sector_data, sizeof(cd_sector.sector_data));
						} catch (...) {
							fprintf(stderr, "Error reading sector %i!\n", sector_index);
							if (sector_index >= 0 && sector_index < (int)sector_count) {
								throw EXIT_FAILURE;
							}
						}
					}
					if (c2_errors) {
						fprintf(stderr, "C2 errors occured during pass\n");
					}
					auto identical_sectors_with_counter_list = std::vector<unsigned int>(max_audio_read_passes);
					for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
						auto cd_sector = track_data.data() + track_data_start_offset + ((sector_index - first_sector) * cd::SECTOR_LENGTH);
						auto &extracted_cdda_sectors = extracted_cdda_sectors_list.at(sector_index - first_sector);
						auto found = false;
						for (auto &extracted_cdda_sector : extracted_cdda_sectors) {
							if (std::memcmp(cd_sector, extracted_cdda_sector.data, cd::SECTOR_LENGTH) == 0) {
								extracted_cdda_sector.counter += 1;
								found = true;
								break;
							}
						}
						if (!found) {
							auto extracted_cdda_sector = ExtractedCDDASector();
							std::memcpy(extracted_cdda_sector.data, cd_sector, cd::SECTOR_LENGTH);
							extracted_cdda_sector.counter += 1;
							extracted_cdda_sectors.push_back(extracted_cdda_sector);
						}
						// Sort in decreasing order.
						std::sort(extracted_cdda_sectors.begin(), extracted_cdda_sectors.end(), [](const ExtractedCDDASector& one, const ExtractedCDDASector& two) -> bool {
							return two.counter < one.counter;
						});
						auto &extracted_cdda_sector = extracted_cdda_sectors.at(0);
						identical_sectors_with_counter_list.at(extracted_cdda_sector.counter - 1) += 1;
					}
					if (audio_pass_index > 0 && identical_sectors_with_counter_list.at(audio_pass_index) == track_length_sectors) {
						fprintf(stderr, "Got %i identical copies of %u sectors\n", audio_pass_index + 1, track_length_sectors);
						break;
					}
				}
				for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
					auto cd_sector = extracted_cdda_sectors_list.at(sector_index - first_sector).at(0);
					auto outcome = image_format->write_sector_data(current_track, cd_sector.data, cd::SECTOR_LENGTH);
					if (!outcome) {
						fprintf(stderr, "Error writing sector data %u to file!\n", sector_index);
						throw EXIT_FAILURE;
					}
				}
			} else {
				fprintf(stderr, "Current track contains data\n");
				auto file_system = iso9660::FileSystem([&](size_t sector, void* user_data) -> void {
					scsi_drive.read_sector(sector, &cd_sector.sector_data, &cd_sector.subchannels_data, &cd_sector.c2_data);
					std::memcpy(user_data, cd_sector.sector_data + data_offset, cdrom::MODE1_DATA_LENGTH);
				});
				fprintf(stderr, "Extracting %u sectors from %u to %u\n", track_length_sectors, first_sector, last_sector - 1);
				for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
					try {
						scsi_drive.read_sector(sector_index, &cd_sector.sector_data, &cd_sector.subchannels_data, &cd_sector.c2_data);
						auto outcome = image_format->write_sector_data(current_track, cd_sector.sector_data + data_offset, data_length);
						if (!outcome) {
							fprintf(stderr, "Error writing sector data %u to file!\n", sector_index);
							throw EXIT_FAILURE;
						}
						if (subchannels) {
							auto outcome2 = image_format->write_subchannel_data(current_track, (UCHAR*)&cd_sector + subchannel_offset);
							if (!outcome2) {
								fprintf(stderr, "Error writing subchannel data %u to file!\n", sector_index);
								throw EXIT_FAILURE;
							}
						}
					} catch (...) {
						fprintf(stderr, "Error reading sector %u!\n", sector_index);
						auto entry = file_system.get_entry(sector_index);
						if (entry) {
							auto& entries = file_system.get_hierarchy(*entry);
							auto identifier = std::string(drive_argument.value()) + ":";
							for (const auto& entry : entries) {
								if (entry.identifier == iso9660::CURRENT_DIRECTORY_IDENTIFIER) {
									continue;
								}
								if (entry.identifier == iso9660::PARENT_DIRECTORY_IDENTIFIER) {
									continue;
								}
								identifier += "\\";
								identifier += std::string(entry.identifier);
							}
							fprintf(stderr, "Sector belongs to \"%s\"\n", identifier.c_str());
						} else {
							fprintf(stderr, "Sector does not belong to specific file.\n");
						}
						bad_sector_numbers.push_back(sector_index);
						auto outcome = image_format->write_sector_data(current_track, empty_cd_sector.sector_data + data_offset, data_length);
						if (!outcome) {
							fprintf(stderr, "Error writing sector data %u to file!\n", sector_index);
							throw EXIT_FAILURE;
						}
						if (subchannels) {
							auto outcome2 = image_format->write_subchannel_data(current_track, (UCHAR*)&cd_sector + subchannel_offset);
							if (!outcome2) {
								fprintf(stderr, "Error writing subchannel data %u to file!\n", sector_index);
								throw EXIT_FAILURE;
							}
						}
					}
				}
			}
		}
		auto duration_ms = get_timestamp_ms() - start_ms;
		fprintf(stderr, "Extraction took %llu seconds\n", duration_ms / 1000);
		image_format->write_index(scsi_drive, toc, toc_ex, subchannels, bad_sector_numbers, track_pregap_sectors_list, track_length_sectors_list);
	}
}

















auto main(
	si_t argc,
	ch08_t** argv
) -> si_t {
	try {
		auto arguments = std::vector<std::string>(argv + 0, argv + argc);
		auto command = arguments.size() >= 2 ? arguments[1] : "";
		if (false) {
		} else if (command == "cue") {
			commands::cue(arguments);
		} else if (command == "iso") {
			commands::iso(arguments, get_handle, pass_through_direct);
		} else if (command == "mds") {
			commands::mds(arguments);
		} else {
			fprintf(stderr, "%s\n", "Expected a valid command!");
		}
		fprintf(stderr, "%s\n", "Program completed successfully.");
		return EXIT_SUCCESS;
	} catch (const std::exception& e) {
		fprintf(stderr, "%s\n", "");
		fprintf(stderr, "%s\n", e.what());
	}
	fprintf(stderr, "%s\n", "Program did not complete successfully!");
	return EXIT_FAILURE;


















	auto command = argc >= 2 ? argv[1] : "";
	if (strcmp(command, "save") == 0) {
		save(argc, argv);
		return EXIT_SUCCESS;
	}
	HANDLE hCD = CreateFileA("\\\\.\\F:", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hCD == NULL) {
		fprintf(stderr, "error opening drive\n");
		return EXIT_FAILURE;
	}
	auto scsi_drive = drive::create_drive(hCD, pass_through_direct);
	auto toc = scsi_drive.read_toc();
	if (false) {
	} else if (strcmp(command, "drive") == 0) {
		STORAGE_PROPERTY_QUERY storagePropertyQuery;
		storagePropertyQuery.PropertyId = StorageDeviceProperty;
		storagePropertyQuery.QueryType = PropertyStandardQuery;

		STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader;
		DWORD dwBytesReturned = 0;
		if (0 == DeviceIoControl(hCD, IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), &storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytesReturned, NULL)) {
			fprintf(stderr, "Device command failed with error %d\n", (int)GetLastError());
			return EXIT_FAILURE;
		}

		DWORD dwOutBufferSize = storageDescriptorHeader.Size;
		unsigned char* pOutBuffer = new unsigned char[dwOutBufferSize];

		if (0 == DeviceIoControl(hCD, IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), pOutBuffer, dwOutBufferSize, &dwBytesReturned, NULL)) {
			fprintf(stderr, "Device command failed with error %d\n", (int)GetLastError());
			return EXIT_FAILURE;
		}

		STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuffer;

		auto vendor_id_offset = pDeviceDescriptor->VendorIdOffset;
		auto vendor_id = "";
		if (vendor_id_offset != 0) {
			vendor_id = (char*)(&pOutBuffer[vendor_id_offset]);
			for (auto i = (int)strlen(vendor_id) - 1; i >= 0; i--) {
				if (vendor_id[i] == ' ') {
					(char&)(vendor_id)[i] = '\0';
				} else {
					break;
				}
			}
		}
		auto product_id_offset = pDeviceDescriptor->ProductIdOffset;
		auto product_id = "";
		if (product_id_offset != 0) {
			product_id = (char*)(&pOutBuffer[product_id_offset]);
			for (auto i = (int)strlen(product_id) - 1; i >= 0; i--) {
				if (product_id[i] == ' ') {
					(char&)(product_id)[i] = '\0';
				} else {
					break;
				}
			}
		}
		fprintf(stderr, "{\"vendor_id\":\"%s\",\"product_id\":\"%s\"}", vendor_id, product_id);
		delete [] pOutBuffer;
		return EXIT_SUCCESS;
	} else if (strcmp(command, "toc") == 0) {
		_setmode(0, _O_BINARY);
		_setmode(1, _O_BINARY);
		fwrite(&toc, 1, sizeof(toc), stdout);
		return EXIT_SUCCESS;
	} else if (strcmp(command, "ext") == 0) {
		if (argc != 3) {
			fprintf(stderr, "Commnand \"ext\" needs a third argument.\n");
			return EXIT_FAILURE;
		}
		auto first_track = toc.header.first_track_or_session_number;
		auto last_track = toc.header.last_track_or_session_number;
		if (strcmp(argv[2], "all") != 0) {
			auto tn = atoi(argv[2]);
			if (tn < toc.header.first_track_or_session_number || tn > toc.header.last_track_or_session_number) {
				fprintf(stderr, "bad track number\n");
				return EXIT_FAILURE;
			}
			first_track = tn;
			last_track = tn;
		}
		_setmode(0, _O_BINARY);
		_setmode(1, _O_BINARY);
		for (auto i = first_track; i <= last_track; i += 1) {
			if ((toc.entries[i-1].control & 0x04) == 0x04) {
				continue;
			}
			int first_sector = cd::get_sector_from_address(toc.entries[i-1].track_start_address) - 150;
			int last_sector = cd::get_sector_from_address(toc.entries[i].track_start_address) - 150;
			if ((toc.entries[i].control & 0x04) == 0x04) {
				last_sector -= (150 + 2) * 75;
			}
			int sectors_to_read = last_sector - first_sector;
			fprintf(stderr, "Reading sectors %d to %d.\n", (int)first_sector, (int)last_sector);
			RAW_READ_INFO ReadInfo;
			ReadInfo.TrackMode = CDDA;
			char* pBuf = new char[sectors_to_read*98*24];
			int sector_offset = first_sector;
			while (true) {
				ReadInfo.DiskOffset.QuadPart = sector_offset * 2048;
				ReadInfo.SectorCount = last_sector - sector_offset - 1;
				// This value is arbitrarily set to 50. Some drives cannot read 50 sectors at once.
				if (ReadInfo.SectorCount > 25) {
					ReadInfo.SectorCount = 25;
				}
				if (ReadInfo.SectorCount == 0) {
					break;
				}
				auto bytes_expected = ReadInfo.SectorCount*98*24;
				ULONG Dummy;
				if (0 == DeviceIoControl(hCD, IOCTL_CDROM_RAW_READ, &ReadInfo, sizeof(ReadInfo), &(pBuf[(sector_offset-first_sector)*98*24]), bytes_expected, &Dummy, NULL)) {
					fprintf(stderr, "error in reading track, error %d, got bytes %d\n", (int)GetLastError(), (int)Dummy);
					return EXIT_FAILURE;
				}
				if (bytes_expected != Dummy) {
					fprintf(stderr, "expected %d bytes, got %d bytes\n", (int)bytes_expected, (int)Dummy);
					return EXIT_FAILURE;
				}
				sector_offset += ReadInfo.SectorCount;
			}
			fwrite(pBuf, sectors_to_read, 98*24, stdout);
		}
		return EXIT_SUCCESS;
	} else {
		fprintf(stderr, "Pleasy specify command.\n");
	}
	return EXIT_FAILURE;
}
