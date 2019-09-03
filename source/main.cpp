#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "fcntl.h"
#include "io.h"


#define CD_RAW_READ_C2_SIZE                    (     294   )
#define CD_RAW_READ_SUBCODE_SIZE               (         98)
#define CD_RAW_SECTOR_WITH_C2_SIZE             (2352+294   )
#define CD_RAW_SECTOR_WITH_SUBCODE_SIZE        (2352    +98)
#define CD_RAW_SECTOR_WITH_C2_AND_SUBCODE_SIZE (2352+294+98)

typedef struct _TRACK_DATA
{
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA;

typedef struct _CDROM_TOC
{
    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;
    TRACK_DATA TrackData[100];
} CDROM_TOC;

ULONG AddressToSectors( UCHAR Addr[4] )
{
    ULONG Sectors = Addr[1]*75*60 + Addr[2]*75 + Addr[3];
    return Sectors - 150;
}

struct CDTRACK
{
    ULONG Address;
    ULONG Length;
};

typedef enum _TRACK_MODE_TYPE {
  YellowMode2,
  XAForm2,
  CDDA,
  RawWithC2AndSubCode,
  RawWithC2,
  RawWithSubCode
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct __RAW_READ_INFO {
  LARGE_INTEGER   DiskOffset;
  ULONG           SectorCount;
  TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;

char *strdup(const char *s)
{
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

auto read_toc(CDROM_TOC& toc, HANDLE handle)
	-> bool
{
	ULONG Dummy;
	memset(&toc, 0, sizeof(toc));
	return (0 != DeviceIoControl(handle, 0x24000, nullptr, 0, &toc, sizeof(toc), &Dummy, NULL));
}

int main(int argc, char** argv) {
	if (argc < 2) {
		return EXIT_FAILURE;
	}
	HANDLE hCD = CreateFile("\\\\.\\F:", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hCD == NULL) {
		fprintf(stderr, "error opening drive\n");
		return EXIT_FAILURE;
	}
	CDROM_TOC toc;
	if (!read_toc(toc, hCD)) {
		fprintf(stderr, "error reading toc, error %d\n", (int)GetLastError());
		return EXIT_FAILURE;
	}
	auto l = ((toc.Length[0] << 8) | (toc.Length[1]));
	if (l > (int)(sizeof(toc) - 2)) {
		fprintf(stderr, "bad toc size\n");
		return EXIT_FAILURE;
	}
	auto op = argv[1];
	if (false) {
	} else if (strcmp(op, "drive") == 0) {
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
	} else if (strcmp(op, "toc") == 0) {
		_setmode(0, _O_BINARY);
		_setmode(1, _O_BINARY);
		fwrite(&toc, 1, l + 2, stdout);
		return EXIT_SUCCESS;
	} else if (strcmp(op, "ext") == 0) {
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
				if (ReadInfo.SectorCount > 50) {
					ReadInfo.SectorCount = 50;
				}
				if (ReadInfo.SectorCount == 0) {
					break;
				}
				auto bytes_expected = ReadInfo.SectorCount*98*24;
				ULONG Dummy;
				if (0 == DeviceIoControl(hCD, 0x2403e, &ReadInfo, sizeof(ReadInfo), &(pBuf[(sector_offset-first_sector)*98*24]), bytes_expected, &Dummy, NULL)) {
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
