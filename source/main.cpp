#include <cstdio>
#include <cmath>
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

#include "idiv.h"
#include "cdb.h"

#define APP_NAME "Disc Reader"
#define APP_VERSION "0.0.0"

#define CD_SECTOR_LENGTH 2352
#define CD_SECTORS_PER_SECOND 75
#define CD_SUBCHANNELS_LENGTH 96

#define CDDA_STEREO_SAMPLE_LENGTH 4
#define CDDA_STEREO_SAMPLES_PER_FRAME 6
#define CDDA_STEREO_FRAME_LENGTH (CDDA_STEREO_SAMPLES_PER_FRAME * CDDA_STEREO_SAMPLE_LENGTH)
#define CDDA_STEREO_FRAMES_PER_SECTOR (CD_SECTOR_LENGTH / CDDA_STEREO_FRAME_LENGTH)

#define CDROM_SYNC_LENGTH 12
#define CDROM_HEADER_LENGTH 4
#define CDROM_SYNC_HEADER_LENGTH (CDROM_SYNC_LENGTH + CDROM_HEADER_LENGTH)
#define CDROM_EDC_LENGTH 4
#define CDROM_PAD_LENGTH 8
#define CDROM_P_PARTIY_LENGTH 172
#define CDROM_Q_PARITY_LENGTH 104
#define CDROM_ECC_LENGTH (CDROM_P_PARTIY_LENGTH + CDROM_Q_PARITY_LENGTH)
#define CDROM_EDC_PAD_ECC_LENGTH (CDROM_EDC_LENGTH + CDROM_PAD_LENGTH + CDROM_ECC_LENGTH)
#define CDROM_XA_SUBHEADER_LENGTH 4
#define CDROM_MODE1_DATA_LENGTH (CD_SECTOR_LENGTH - CDROM_SYNC_HEADER_LENGTH - CDROM_EDC_PAD_ECC_LENGTH)
#define CDROM_MODE2_DATA_LENGTH (CD_SECTOR_LENGTH - CDROM_SYNC_HEADER_LENGTH)
#define CDROM_MODE2_FORM_1_DATA_LENGTH (CD_SECTOR_LENGTH - CDROM_SYNC_HEADER_LENGTH - CDROM_XA_SUBHEADER_LENGTH - CDROM_XA_SUBHEADER_LENGTH - CDROM_EDC_LENGTH - CDROM_ECC_LENGTH)
#define CDROM_MODE2_FORM_2_DATA_LENGTH (CD_SECTOR_LENGTH - CDROM_SYNC_HEADER_LENGTH - CDROM_XA_SUBHEADER_LENGTH - CDROM_XA_SUBHEADER_LENGTH- CDROM_EDC_LENGTH)

auto byteswap16(uint16_t v)
-> uint16_t {
	auto pointer = (uint8_t*)&v;
	return (pointer[0] << 8) | (pointer[1] << 0);
}

auto byteswap32(uint32_t v)
-> uint32_t {
	auto pointer = (uint8_t*)&v;
	return (pointer[0] << 24) | (pointer[1] << 16) | (pointer[2] << 8) | (pointer[3] << 0);
}

auto ascii_dump(uint8_t* bytes, int size, const char* name)
-> void {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s:\n", name);
	auto rows = (size + 15) / 16;
	auto offset = 0;
	auto remaining_bytes = size;
	for (auto r = 0; r < rows; r += 1) {
		fprintf(stderr, "%.8X: ", offset);
		auto cols = remaining_bytes <= 16 ? remaining_bytes : 16;
		for (auto c = 0; c < cols; c += 1) {
			auto byte = bytes[offset + c];
			fprintf(stderr, "%.2X ", byte);
		}
		for (auto c = cols; c < 16; c += 1) {
			fprintf(stderr, "   ");
		}
		for (auto c = 0; c < cols; c += 1) {
			auto byte = bytes[offset + c];
			fprintf(stderr, "%c", byte >= 32 && byte <= 127 ? byte : '.');
		}
		for (auto c = cols; c < 16; c += 1) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");
		offset += cols;
		remaining_bytes -= cols;
	}
	fprintf(stderr, "\n");
}

#define DUMP(var) ascii_dump((uint8_t*)(void*)&var, sizeof(var), #var)

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

namespace cdrom {
	#pragma pack(push, 1)

	typedef struct {
		uint8_t file_number;
		uint8_t channel_number;
		uint8_t submode; // Bit 5 indicates form1 when set to 0 and form2 when set to 1.
		uint8_t coding_info;
	} XASubheader;

	typedef struct {
		uint8_t channels[8][12];
	} SubchannelData;

	enum class Control: uint8_t {
		AUDIO_2_CHANNELS_COPY_PROTECTED = 0b0000,
		AUDIO_2_CHANNELS_WITH_PRE_EMPHASIS_COPY_PROTECTED = 0b0001,
		AUDIO_4_CHANNELS_COPY_PROTECTED = 0b1000,
		AUDIO_4_CHANNELS_WITH_PRE_EMPHASIS_COPY_PROTECTED = 0b1001,
		DATA_RECORD_UINTERRUPTED_COPY_PROTECTED = 0b0100,
		DATA_RECORDED_INCREMENTALLY_COPY_PROTECTED = 0b0101,
		AUDIO_2_CHANNELS_COPY_PERMITTED = 0b0010,
		AUDIO_2_CHANNELS_WITH_PRE_EMPHASIS_COPY_PERMITTED = 0b0011,
		AUDIO_4_CHANNELS_COPY_PERMITTED = 0b1010,
		AUDIO_4_CHANNELS_WITH_PRE_EMPHASIS_COPY_PERMITTED = 0b1011,
		DATA_RECORD_UINTERRUPTED_COPY_PERMITTED = 0b0110,
		DATA_RECORDED_INCREMENTALLY_COPY_PERMITTED = 0b0111
	};

	typedef struct {
		uint8_t adr: 4;
		uint8_t control: 4;
		union {
			struct {
				uint8_t track_number;
				uint8_t track_index;
				uint8_t relative_m_bcd;
				uint8_t relative_s_bcd;
				uint8_t relative_f_bcd;
				uint8_t zero = 0;
				uint8_t absolute_m_bcd;
				uint8_t absolute_s_bcd;
				uint8_t absolute_f_bcd;
			} mode1;
		};
		uint16_t crc_be;
	} SubchannelQ;

	#pragma pack(pop)
}

ULONG AddressToSectors(UCHAR Addr[4]) {
	ULONG Sectors = Addr[1]*75*60 + Addr[2]*75 + Addr[3];
	return Sectors - 150;
}

auto  get_timestamp_ms()
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

char *strdup(const char *s) {
	size_t slen = strlen(s);
	char *result = (char*)malloc(slen + 1);
	if (result == NULL)
	{
		return NULL;
	}

	memcpy(result, s, slen + 1);
	return result;
}

namespace libjson {
	class element_t {
		public:
			virtual ~element_t() = 0;
		protected:
		private:
	};

	class string_t: public element_t {
		public:
			string_t(const char* string);
		protected:
		private:
			char* string;
			int length;
	};

	string_t::string_t(const char* string)
		: string(strdup(string))
		, length(strlen(string)) {

	}
	auto parse(const char* string)
		-> element_t* {
		return string == nullptr ? nullptr : nullptr;
	}

	element_t::~element_t() {

	}
}

typedef struct {
	UCHAR data[CD_SECTOR_LENGTH];
	UCHAR subchannel_data[CD_SUBCHANNELS_LENGTH];
} CD_SECTOR_DATA;



auto get_cdrom_handle(std::string &drive)
-> HANDLE {
	auto filename = std::string("\\\\.\\") + drive + ":";
	SetLastError(ERROR_SUCCESS);
	auto handle = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	WINAPI_CHECK_STATUS();
	return handle;
}

auto get_cdrom_toc(HANDLE handle)
-> CDROM_TOC {
	auto toc = CDROM_TOC();
	auto bytes_returned = ULONG();
	SetLastError(ERROR_SUCCESS);
	DeviceIoControl(handle, IOCTL_CDROM_READ_TOC, nullptr, 0, &toc, sizeof(toc), &bytes_returned, nullptr);
	WINAPI_CHECK_STATUS();
	return toc;
}

auto validate_cdrom_toc(CDROM_TOC &toc)
-> void {
	auto length = (toc.Length[0] << 8) | (toc.Length[1] << 0);
	auto max_length = (int)sizeof(toc) - 2;
	if (length > max_length) {
		fprintf(stderr, "Expected TOC length of %i to be at most %i!\n", length, max_length);
		throw EXIT_FAILURE;
	}
	if (toc.FirstTrack < 1 || toc.FirstTrack > 99) {
		fprintf(stderr, "Expected first track %i to be a value between 1 and 99!\n", toc.FirstTrack);
		throw EXIT_FAILURE;
	}
	if (toc.LastTrack < 1 || toc.LastTrack > 99) {
		fprintf(stderr, "Expected last track %i to be a value between 1 and 99!\n", toc.LastTrack);
		throw EXIT_FAILURE;
	}
	auto number_of_tracks = toc.LastTrack - toc.FirstTrack + 1;
	if (number_of_tracks < 1) {
		fprintf(stderr, "Expected the disc to contain at least one track!\n");
		throw EXIT_FAILURE;
	}
}

typedef struct {
	SCSI_PASS_THROUGH_DIRECT sptd;
	UCHAR sense[255];
} SPTDWithSenseBuffer;

auto read_sector_sptd(
	HANDLE handle,
	CD_SECTOR_DATA& sector,
	ULONG lba
) -> void {
	memset(&sector, 0, sizeof(sector));
	auto data = SPTDWithSenseBuffer();
	auto cdb = cdb::ReadCD12();
	cdb.expected_sector_type = cdb::ReadCD12ExpectedSectorType::ANY;
	cdb.lba_be = byteswap32(lba);
	cdb.transfer_length_be[2] = 1;
	cdb.errors = cdb::ReadCD12Errors::NONE;
	cdb.edc_and_ecc = 1;
	cdb.user_data = 1;
	cdb.header_codes = cdb::ReadCD12HeaderCodes::ALL_HEADERS;
	cdb.sync = 1;
	cdb.subchannel_selection_bits = cdb::ReadCD12SubchanelBits::RAW;
	data.sptd.Length = sizeof(data.sptd);
	data.sptd.CdbLength = sizeof(cdb);
	data.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	data.sptd.TimeOutValue = 10;
	data.sptd.DataBuffer = &sector;
	data.sptd.DataTransferLength = sizeof(sector);
	data.sptd.SenseInfoLength = sizeof(data.sense);
	data.sptd.SenseInfoOffset = offsetof(SPTDWithSenseBuffer, sense);
	memcpy(data.sptd.Cdb, &cdb, sizeof(cdb));
	auto bytes_returned = ULONG(0);
	auto bytes_expected = ULONG(sizeof(data.sptd));
	SetLastError(ERROR_SUCCESS);
	auto outcome = DeviceIoControl(handle, IOCTL_SCSI_PASS_THROUGH_DIRECT, &data, sizeof(data), &data, sizeof(data), &bytes_returned, nullptr);
	WINAPI_CHECK_STATUS();
	if (!outcome || (bytes_returned != bytes_expected)) {
		throw EXIT_FAILURE;
	}
}

typedef struct {
	cdb::ModeParameterHeader10 parameter_header;
	cdb::ReadWriteErrorRecoveryModePage page_data;
} ModeSense;

auto sptd_mode_sense(
	HANDLE handle,
	ModeSense& mode_sense
) -> void {
	auto data = SPTDWithSenseBuffer();
	auto cdb = cdb::ModeSense10();
	cdb.page_code = cdb::SensePage::ReadWriteErrorRecoveryModePage;
	cdb.allocation_length_be = byteswap16(sizeof(mode_sense));
	data.sptd.Length = sizeof(data.sptd);
	data.sptd.CdbLength = sizeof(cdb);
	data.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	data.sptd.TimeOutValue = 10;
	data.sptd.DataBuffer = &mode_sense;
	data.sptd.DataTransferLength = sizeof(mode_sense);
	data.sptd.SenseInfoLength = sizeof(data.sense);
	data.sptd.SenseInfoOffset = offsetof(SPTDWithSenseBuffer, sense);
	memcpy(data.sptd.Cdb, &cdb, sizeof(cdb));
	auto bytes_returned = ULONG(0);
	auto bytes_expected = ULONG(sizeof(data.sptd));
	SetLastError(ERROR_SUCCESS);
	auto outcome = DeviceIoControl(
		handle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&data,
		sizeof(data),
		&data,
		sizeof(data),
		&bytes_returned,
		nullptr
	);
	WINAPI_CHECK_STATUS();
	if (!outcome || (bytes_returned != bytes_expected)) {
		throw EXIT_FAILURE;
	}
}

auto sptd_mode_select(
	HANDLE handle,
	ModeSense& mode_sense
) -> void {
	auto data = SPTDWithSenseBuffer();
	auto cdb = cdb::ModeSelect10();
	cdb.page_format = 1;
	cdb.parameter_list_length_be = byteswap16(sizeof(mode_sense));
	data.sptd.Length = sizeof(data.sptd);
	data.sptd.CdbLength = sizeof(cdb);
	data.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	data.sptd.TimeOutValue = 10;
	data.sptd.DataBuffer = &mode_sense;
	data.sptd.DataTransferLength = sizeof(mode_sense);
	data.sptd.SenseInfoLength = sizeof(data.sense);
	data.sptd.SenseInfoOffset = offsetof(SPTDWithSenseBuffer, sense);
	memcpy(data.sptd.Cdb, &cdb, sizeof(cdb));
	auto bytes_returned = ULONG();
	auto bytes_expected = ULONG(sizeof(data.sptd));
	SetLastError(ERROR_SUCCESS);
	auto outcome = DeviceIoControl(
		handle,
		IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&data,
		sizeof(data),
		&data,
		sizeof(data),
		&bytes_returned,
		nullptr
	);
	WINAPI_CHECK_STATUS();
	if (!outcome || (bytes_returned != bytes_expected)) {
		throw EXIT_FAILURE;
	}
}

auto read_raw_sector(
	HANDLE handle,
	uint8_t* data,
	ULONG lba
) -> void {
	auto read_info = RAW_READ_INFO();
	read_info.TrackMode = TRACK_MODE_TYPE::CDDA;
	read_info.SectorCount = 1;
	read_info.DiskOffset.QuadPart = lba * CDROM_MODE1_DATA_LENGTH;
	auto bytes_returned = ULONG(0);
	auto bytes_expected = ULONG(CD_SECTOR_LENGTH);
	SetLastError(ERROR_SUCCESS);
	auto outcome = DeviceIoControl(
		handle,
		IOCTL_CDROM_RAW_READ,
		&read_info,
		sizeof(read_info),
		data,
		CD_SECTOR_LENGTH,
		&bytes_returned,
		nullptr
	);
	WINAPI_CHECK_STATUS();
	if (!outcome || (bytes_returned != bytes_expected)) {
		throw EXIT_FAILURE;
	}
}

enum class TrackType {
	AUDIO,
	DATA
};

enum class FileFormat {
	MDF_MDS
};

auto get_track_type(TRACK_DATA &track)
-> TrackType {
	return (track.Control & 0b0100) == 0 ? TrackType::AUDIO : TrackType::DATA;
}

auto write_cd_sector_to_file(CD_SECTOR_DATA &sector, FILE *target_handle, bool subchannels)
-> bool {
	auto bytes_expected = (size_t)(subchannels ? CD_SECTOR_LENGTH + CD_SUBCHANNELS_LENGTH : CD_SECTOR_LENGTH);
	auto bytes_returned = fwrite(&sector, 1, bytes_expected, target_handle);
	return bytes_returned == bytes_expected;
}

auto deinterleave_subchannel_data(CD_SECTOR_DATA &sector)
-> cdrom::SubchannelData {
	auto subchannel_data = cdrom::SubchannelData();
	for (auto subchannel_index = 7; subchannel_index >= 0; subchannel_index -= 1) {
		auto channel_right_shift = 7 - subchannel_index;
		auto offset = 0;
		for (auto byte_index = 0; byte_index < 12; byte_index += 1) {
			auto byte = 0;
			for (auto bit_index = 0; bit_index < 8; bit_index += 1) {
				auto subchannel_byte = sector.subchannel_data[offset];
				auto subchannel_bit = (subchannel_byte >> channel_right_shift) & 1;
				byte <<= 1;
				byte |= subchannel_bit;
				offset += 1;
			}
			subchannel_data.channels[subchannel_index][byte_index] = byte;
		}
	}
	return subchannel_data;
}

namespace mds {
	#pragma pack(push, 1)

	enum class TrackMode: uint8_t {
		UNKNOWN = 0x00,
		AUDIO = 0xA9,
		MODE1 = 0xAA,
		MODE2 = 0xAB,
		MODE2_FORM1 = 0xAC,
		MODE2_FORM2 = 0xAD
	};

	enum class EntryTypeATrackNumber: uint8_t {
		UNKNOWN = 0x00,
		FIRST_TRACK = 0xA0,
		LAST_TRACK = 0xA1,
		LEAD_OUT_TRACK = 0xA2
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
		uint8_t __a[2] = {};
		uint8_t flags;
		uint8_t __b[1] = {};
		EntryTypeATrackNumber track_number;
		uint8_t __c[3] = {};
		uint8_t address_p = 0;
		uint8_t address_m = 0; // Set to track number for A0 and A1.
		uint8_t address_s = 0;
		uint8_t address_f = 0;
		uint8_t __d[68] = {};
	} EntryTypeA;

	typedef struct {
		TrackMode track_mode;
		uint8_t __a[1] = {};
		uint8_t flags;
		uint8_t __b[1] = {};
		uint8_t track_number;
		uint8_t __c[3] = {};
		uint8_t address_p = 0;
		uint8_t address_m = 0;
		uint8_t address_s = 0;
		uint8_t address_f = 0;
		uint32_t absolute_offset_to_track_table_entry;
		uint16_t sector_length = 2352;
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

auto save(int argc, char **argv)
-> void {
	auto subchannels = false;
	auto directory = ".\\";
	auto filename = "image";
	auto format = FileFormat::MDF_MDS;
	auto drive_argument = std::optional<std::string>();
	auto max_read_retries = 8;
	auto read_offset_correction = 0;
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
			// TODO: Strip extension and set format from .mds/.mdf.
			filename = value;
		} else if (strstr(argument, "--format=") == argument) {
			auto value = argument + sizeof("--format=") - 1;
			if (false) {
			} else if (strcmp(value, "MDF/MDS") == 0) {
				format = FileFormat::MDF_MDS;
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
			} else if (parsed_value >= 0 - (10 * CD_SECTOR_LENGTH) && parsed_value <= 0 + (10 * CD_SECTOR_LENGTH)) {
				read_offset_correction = parsed_value;
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
		fprintf(stderr, "\t\ta) MDF/MDS.\n");
		fprintf(stderr, "\t--drive=string\n");
		fprintf(stderr, "\t\tSet optical drive letter.\n");
		fprintf(stderr, "\t--max-read-retries=integer[0,255]\n");
		fprintf(stderr, "\t\tSet max read retries made before producing a read error (8 by default).\n");
		fprintf(stderr, "\t--read-offset-correction=integer[%i,%i]\n", 0 - (CD_SECTOR_LENGTH * 10), 0 + (CD_SECTOR_LENGTH * 10));
		fprintf(stderr, "\t\tSet read offset correction (samples) for audio track extraction (0 by default).\n");
		throw EXIT_FAILURE;
	} else {
		auto handle = get_cdrom_handle(drive_argument.value());
		auto toc = get_cdrom_toc(handle);
		validate_cdrom_toc(toc);
		{
			auto mode_sense = ModeSense();
			sptd_mode_sense(handle, mode_sense);
			mode_sense.page_data.read_retry_count = max_read_retries;
			sptd_mode_select(handle, mode_sense);
		}
		auto &first_track = toc.TrackData[toc.FirstTrack - 1];
		auto &last_track = toc.TrackData[toc.LastTrack - 1];
		auto &lead_out_track = toc.TrackData[toc.LastTrack + 1 - 1];
		auto track_count = toc.LastTrack - toc.FirstTrack + 1;
		auto sector_count = AddressToSectors(lead_out_track.Address) - AddressToSectors(first_track.Address);
		fprintf(stderr, "Disc contains %i tracks\n", track_count);
		fprintf(stderr, "Disc length is %lu sectors\n", sector_count);
		auto target_path_mdf = std::string(directory) + std::string(filename) + std::string(".mdf");
			auto target_handle_mdf = fopen(target_path_mdf.c_str(), "wb+");
		if (target_handle_mdf == nullptr) {
			fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_mdf.c_str());
			throw EXIT_FAILURE;
		}
		auto target_path_mds = std::string(directory) + std::string(filename) + std::string(".mds");
		auto target_handle_mds = fopen(target_path_mds.c_str(), "wb+");
		if (target_handle_mds == nullptr) {
			fprintf(stderr, "Failed opening file \"%s\"!\n", target_path_mds.c_str());
			throw EXIT_FAILURE;
		}
		auto read_offset_correction_bytes = read_offset_correction * CDDA_STEREO_SAMPLE_LENGTH;
		fprintf(stderr, "Read offset correction is set to %i samples (%i bytes)\n", read_offset_correction, read_offset_correction_bytes);
		auto track_pregap_sectors_list = std::vector<unsigned int>();
		track_pregap_sectors_list.push_back(2 * CD_SECTORS_PER_SECOND);
		for (auto i = toc.FirstTrack + 1; i <= toc.LastTrack; i += 1) {
			auto &last_track = toc.TrackData[i - 1 - 1];
			auto &current_track = toc.TrackData[i - 1];
			auto last_track_type = get_track_type(last_track);
			auto current_track_type = get_track_type(current_track);
			auto track_type_change = current_track_type != last_track_type;
			auto track_pregap_seconds = track_type_change ? 2 : 0;
			auto track_pregap_sectors = track_pregap_seconds * CD_SECTORS_PER_SECOND;
			track_pregap_sectors_list.push_back(track_pregap_sectors);
		}
		auto track_length_sectors_list = std::vector<unsigned int>();
		auto bad_sector_numbers = std::vector<int>();
		auto empty_cd_sector = CD_SECTOR_DATA();
		auto cd_sector = CD_SECTOR_DATA();
		auto start_ms = get_timestamp_ms();
		for (auto i = toc.FirstTrack; i <= toc.LastTrack; i += 1) {
			fprintf(stderr, "Processing track %u\n", i);
			auto &current_track = toc.TrackData[i - 1];
			auto &next_track = toc.TrackData[i + 1 - 1];
			auto current_track_lba = AddressToSectors(current_track.Address);
			auto next_track_lba = AddressToSectors(next_track.Address);
			fprintf(stderr, "Current track starts at sector %lu\n", current_track_lba);
			fprintf(stderr, "Next track starts at sector %lu\n", next_track_lba);
			auto next_track_pregap_sectors = i == toc.LastTrack ? 0 : track_pregap_sectors_list.at(i + 1 -  toc.FirstTrack);
			fprintf(stderr, "Next track has a pregap of %i sectors\n", next_track_pregap_sectors);
			auto first_sector = current_track_lba;
			auto last_sector = next_track_lba - next_track_pregap_sectors;
			auto track_length_sectors = last_sector - first_sector;
			track_length_sectors_list.push_back(track_length_sectors);
			auto current_track_type = get_track_type(current_track);
			if (current_track_type == TrackType::AUDIO) {
				fprintf(stderr, "Current track contains audio\n");
				auto start_offset_bytes = (first_sector * CD_SECTOR_LENGTH) + read_offset_correction_bytes;
				auto end_offset_bytes = (last_sector * CD_SECTOR_LENGTH) + read_offset_correction_bytes;
				auto adjusted_first_sector = idiv_floor(start_offset_bytes, CD_SECTOR_LENGTH);
				auto adjusted_last_sector = idiv_ceil(end_offset_bytes, CD_SECTOR_LENGTH);
				auto adjusted_track_length_sectors = adjusted_last_sector - adjusted_first_sector;
				fprintf(stderr, "Extracting to %i sectors from %i (inclusive) to %i (exclusive)\n", adjusted_track_length_sectors, adjusted_first_sector, adjusted_last_sector);
				auto track_data = std::vector<uint8_t>(adjusted_track_length_sectors * CD_SECTOR_LENGTH);
				for (auto sector_index = adjusted_first_sector; sector_index < adjusted_last_sector; sector_index += 1) {
					try {
						read_raw_sector(handle, track_data.data() + (sector_index - adjusted_first_sector) * CD_SECTOR_LENGTH, sector_index);
					} catch (...) {
						fprintf(stderr, "Error reading sector %i!\n", sector_index);
						if (sector_index >= 0 && sector_index < (int)sector_count) {
							throw EXIT_FAILURE;
						}
					}
				}
				auto track_data_start_offset = read_offset_correction_bytes - ((adjusted_first_sector - first_sector) * CD_SECTOR_LENGTH);
				fprintf(stderr, "Discarding the first %lu bytes of sector %i and the last %lu bytes of sector %i\n", track_data_start_offset, adjusted_first_sector, track_data_start_offset, adjusted_last_sector);
				for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
					auto cd_sector = track_data.data() + track_data_start_offset + ((sector_index - first_sector) * CD_SECTOR_LENGTH);
					auto outcome = write_cd_sector_to_file(*(CD_SECTOR_DATA*)cd_sector, target_handle_mdf, false);
					if (!outcome) {
						fprintf(stderr, "Error writing sector %lu to file \"%s\"!\n", sector_index, target_path_mdf.c_str());
						throw EXIT_FAILURE;
					}
				}
			} else {
				fprintf(stderr, "Current track contains data\n");
				fprintf(stderr, "Extracting %lu sectors from %lu (inclusive) to %lu (exclusive)\n", track_length_sectors, first_sector, last_sector);
				for (auto sector_index = first_sector; sector_index < last_sector; sector_index += 1) {
					try {
						read_sector_sptd(handle, cd_sector, sector_index);
						auto outcome = write_cd_sector_to_file(cd_sector, target_handle_mdf, subchannels);
						if (!outcome) {
							fprintf(stderr, "Error writing sector %lu to file \"%s\"!\n", sector_index, target_path_mdf.c_str());
							throw EXIT_FAILURE;
						}
					} catch (...) {
						fprintf(stderr, "Error reading sector %lu!\n", sector_index);
						bad_sector_numbers.push_back(sector_index);
						auto outcome = write_cd_sector_to_file(empty_cd_sector, target_handle_mdf, subchannels);
						if (!outcome) {
							fprintf(stderr, "Error writing sector %lu to file \"%s\"!\n", sector_index, target_path_mdf.c_str());
							throw EXIT_FAILURE;
						}
					}
				}
			}
		}
		auto duration_ms = get_timestamp_ms() - start_ms;
		fprintf(stderr, "Extraction took %llu milliseconds\n", duration_ms);
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
		auto first_track_entry = mds::EntryTypeA();
		first_track_entry.flags = first_track.Adr << 4 | first_track.Control;
		first_track_entry.track_number = mds::EntryTypeATrackNumber::FIRST_TRACK;
		first_track_entry.address_m = toc.FirstTrack;
		if (fwrite(&first_track_entry, sizeof(first_track_entry), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing first track entry!\n");
			throw EXIT_FAILURE;
		}
		auto last_track_entry = mds::EntryTypeA();
		last_track_entry.flags = last_track.Adr << 4 | last_track.Control;
		last_track_entry.track_number = mds::EntryTypeATrackNumber::LAST_TRACK;
		last_track_entry.address_m = toc.LastTrack;
		if (fwrite(&last_track_entry, sizeof(last_track_entry), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing last track entry!\n");
			throw EXIT_FAILURE;
		}
		auto lead_out_track_entry = mds::EntryTypeA();
		lead_out_track_entry.flags = lead_out_track.Adr << 4 | lead_out_track.Control;
		lead_out_track_entry.track_number = mds::EntryTypeATrackNumber::LEAD_OUT_TRACK;
		lead_out_track_entry.address_p = lead_out_track.Address[0];
		lead_out_track_entry.address_m = lead_out_track.Address[1];
		lead_out_track_entry.address_s = lead_out_track.Address[2];
		lead_out_track_entry.address_f = lead_out_track.Address[3];
		if (fwrite(&lead_out_track_entry, sizeof(lead_out_track_entry), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing lead out track entry!\n");
			throw EXIT_FAILURE;
		}
		auto first_sector_on_disc = 0;
		auto mdf_byte_offset = 0;
		for (auto i = toc.FirstTrack; i <= toc.LastTrack; i += 1) {
			auto &current_track = toc.TrackData[i - 1];
			auto current_track_type = get_track_type(current_track);
			auto current_track_entry = mds::EntryTypeB();
			// TODO: Improve mode detection.
			current_track_entry.track_mode = current_track_type == TrackType::AUDIO ? mds::TrackMode::AUDIO : mds::TrackMode::MODE1;
			current_track_entry.flags = current_track.Adr << 4 | current_track.Control;
			current_track_entry.track_number = i;
			current_track_entry.sector_length = subchannels ? CD_SECTOR_LENGTH + CD_SUBCHANNELS_LENGTH : CD_SECTOR_LENGTH;
			current_track_entry.address_p = current_track.Address[0];
			current_track_entry.address_m = current_track.Address[1];
			current_track_entry.address_s = current_track.Address[2];
			current_track_entry.address_f = current_track.Address[3];
			current_track_entry.first_sector_on_disc = first_sector_on_disc;
			current_track_entry.mdf_byte_offset = mdf_byte_offset;
			current_track_entry.absolute_offset_to_track_table_entry = absolute_offset_to_track_table_entry + (i - toc.FirstTrack) * sizeof(mds::TrackTableEntry);
			current_track_entry.absolute_offset_to_file_table_header = absolute_offset_to_file_table_header;
			if (fwrite(&current_track_entry, sizeof(current_track_entry), 1, target_handle_mds) != 1) {
				fprintf(stderr, "Error writing current track entry!\n");
				throw EXIT_FAILURE;
			}
			auto current_track_length_sectors = track_length_sectors_list.at(i - toc.FirstTrack);
			auto next_track_pregap_sectors = i == toc.LastTrack ? 0 : track_pregap_sectors_list.at(i + 1 - toc.FirstTrack);
			first_sector_on_disc += current_track_length_sectors + next_track_pregap_sectors;
			mdf_byte_offset += current_track_length_sectors * current_track_entry.sector_length;
		}
		auto track_table_header = mds::TrackTableHeader();
		if (fwrite(&track_table_header, sizeof(track_table_header), 1, target_handle_mds) != 1) {
			fprintf(stderr, "Error writing track table header!\n");
			throw EXIT_FAILURE;
		}
		for (auto i = toc.FirstTrack; i <= toc.LastTrack; i += 1) {
			auto track_table_entry = mds::TrackTableEntry();
			track_table_entry.pregap_sectors = track_pregap_sectors_list.at(i - toc.FirstTrack);
			track_table_entry.length_sectors = track_length_sectors_list.at(i - toc.FirstTrack);
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
}

auto main(int argc, char **argv)
-> int {
	auto command = argc >= 2 ? argv[1] : "";
	if (strcmp(command, "save") == 0) {
		try {
			save(argc, argv);
			fprintf(stderr, "Program completed successfully.\n");
			return EXIT_SUCCESS;
		} catch (...) {
			fprintf(stderr, "Program did not complete successfully!\n");
			return EXIT_FAILURE;
		}
	}


	HANDLE hCD = CreateFileA("\\\\.\\F:", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hCD == NULL) {
		fprintf(stderr, "error opening drive\n");
		return EXIT_FAILURE;
	}
	auto toc = get_cdrom_toc(hCD);
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
		auto first_track = toc.FirstTrack;
		auto last_track = toc.LastTrack;
		if (strcmp(argv[2], "all") != 0) {
			auto tn = atoi(argv[2]);
			if (tn < toc.FirstTrack || tn > toc.LastTrack) {
				fprintf(stderr, "bad track number\n");
				return EXIT_FAILURE;
			}
			first_track = tn;
			last_track = tn;
		}
		_setmode(0, _O_BINARY);
		_setmode(1, _O_BINARY);
		for (auto i = first_track; i <= last_track; i += 1) {
			if ((toc.TrackData[i-1].Control & 0x04) == 0x04) {
				continue;
			}
			int first_sector = AddressToSectors(toc.TrackData[i-1].Address);
			int last_sector = AddressToSectors(toc.TrackData[i].Address);
			if ((toc.TrackData[i].Control & 0x04) == 0x04) {
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
