# Overdrive

Low-level utility for creating disc images from data, audio and mixed-mode optical discs.

```
overdrive cue F: image.cue
```

## Tasks

Overdrive can be used to create optical disc images in several different file formats through its various tasks. Each task comes with its own set of options which may be displayed through the `overdrive <task>` command.

### BIN/CUE

Create a disc image with the BIN/CUE file format through the `cue` task. The format is recommended for unprotected discs containing audio tracks.

```
overdrive cue F: image.cue
```

### ISO

Create a disc image with the ISO file format through the `iso` task. The format is recommended for unprotected discs containing a single data track.

```
overdrive iso F: image.iso
```

### MDF/MDS

Create a disc image with the MDF/MDS file format through the `mds` task. The format is recommended for discs containing all types of content.

```
overdrive mds F: image.mds
```

### ODI

Create a disc image with the ODI file format through the `odi` task. The format is recommended for discs containing all types of content.

```
overdrive odi F: image.odi
```

## The ODI file format

[TODO: First sector, lead in lead out]

The ODI file format is Overdrive's own compact, highly-flexible and freely available disc image format. Overdrive includes a [reference implementation](./source/lib/odi.h).

The format is semantically versioned, implying that breaking changes trigger a new major version while backward-compatible changes trigger a new minor version. This specification details the `1.0` version of the format.

Little-endian data types are exclusively used within the format meaning that the number `256` is encoded as the four byte sequence `0x00 0x01 0x00 0x00`. All offsets are specified as absolute offsets (i.e. relative to the beginining of the image file). They are currently defined to be 32-bit wide implying a maximum file size of 4 GB. However, additional space is reserved in the format allowing all offsets to be extended to at most 48-bit wide.

### The file header

The disc image is stored in a single file which starts with a `FileHeader` structure. The structure contains a `header_length` member, indicating the total number of bytes occupied by the `FileHeader` structure. This value may be increased in future versions of the format. Implementations are expected to be able to handle this transparently.

```c++
struct FileHeader {
	ch08_t identifier[16] = "OVERDRIVE IMAGE"; // Zero-terminated.
	ui08_t major_version = MAJOR_VERSION;
	ui08_t minor_version = MINOR_VERSION;
	ui16_t header_length = sizeof(FileHeader);
	ui32_t sector_table_header_absolute_offset;
	ui08_t : 8;
	ui08_t : 8;
	ui32_t point_table_header_absolute_offset;
	ui08_t : 8;
	ui08_t : 8;
};
```

The `sector_table_header_absolute_offset` member indicates the absolute offset of the `SectorTableHeader` structure and the `point_table_header_absolute_offset` member indicates the absolute offset of the `PointTableHeader` structure. The sector table contains information about the sectors stored in the image while the point table contains information about the points stored in the image. The points can be used to derive information about the sessions and tracks of the optical disc used to create the image.

### The sector table

The `SectorTableHeader` structure contains three fields which are all required to read entries from the sector table.

The structure contains a `header_length` member, indicating the total number of bytes occupied by the `SectorTableHeader` structure. The structure also contains an `entry_length` member, indicating the total number of bytes occupied by each of the `SectorTableEntry` structures. Both values may be increased in future versions of the format. Implementations are expected to be able to handle this transparently.

The `SectorTableEntry` structures are located immediately after the header. The `entry_count` member indicates the total number of structures.

```c++
struct SectorTableHeader {
	ui16_t header_length = sizeof(SectorTableHeader);
	ui16_t entry_length = sizeof(SectorTableEntry);
	ui32_t entry_count;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
};
```

Each `SectorTableEntry` structure provides information about a single sector. The `compressed_data_absolute_offset` member specifies the absolute offset of the compressed data. All compressed data is stored packed at the offset in question Sector data followed by subchannels data.

The `readability` member specifies whether or not the sector was successfully read from the optical disc. Valid options are `UNREADABLE (0x00)` and `READABLE (0x01)`.

```c++
struct SectorTableEntry {
	ui32_t compressed_data_absolute_offset;
	ui08_t : 8;
	ui08_t : 8;
	Readability::type readability;
	SectorDataCompressionHeader sector_data;
	SubchannelsDataCompressionHeader subchannels_data;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
};
```

The `sector_data` member contains an embedded `SectorDataCompressionHeader` structure whose members `compression_method` and `compressed_byte_count` specify the compression method used to store the sector data and the resulting compressed byte count.

Valid options for the `compression_method` member is `NONE (0x00)`, `RUN_LENGTH_ENCODING (0x01)` and `LOSSLESS_STEREO_AUDIO (0x80)`. The compression method `NONE (0x00)` indicates that the data is stored uncompressed and that `compressed_byte_count` bytes can be read directly. The other compression algorithms are detailed in the `Compression methods` section of this document.

```c++
struct SectorDataCompressionHeader {
	SectorDataCompressionMethod::type compression_method;
	ui16_t compressed_byte_count;
};
```

The `subchannels_data` member contains an embedded `SubchannelsDataCompressionHeader` structure whose members `compression_method` and `compressed_byte_count` specify the compression method used to store the subchannels data and the resulting compressed byte count.

Valid options for the `compression_method` member is `NONE (0x00)` and `RUN_LENGTH_ENCODING (0x01)`. The compression method `NONE (0x00)` indicates that the data is stored uncompressed and that `compressed_byte_count` bytes can be read directly. The other compression algorithms are detailed in the `Compression methods` section of this document.

```c++
struct SubchannelsDataCompressionHeader {
	SubchannelsDataCompressionMethod::type compression_method;
	ui16_t compressed_byte_count;
};
```

## The point table

The `PointTableHeader` structure contains three fields which are all required to read entries from the point table.

The structure contains a `header_length` member, indicating the total number of bytes occupied by the `PointTableHeader` structure. The structure also contains an `entry_length` member, indicating the total number of bytes occupied by each of the `PointTableEntry` structures. Both values may be increased in future versions of the format. Implementations are expected to be able to handle this transparently.

The `PointTableEntry` structures are located immediately after the header. The `entry_count` member indicates the total number of structures.

```c++
struct PointTableHeader {
	ui16_t header_length = sizeof(PointTableHeader);
	ui16_t entry_length = sizeof(PointTableEntry);
	ui32_t entry_count;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
};
```

Each `PointTableEntry` structure provides information about a single point. The `entry` member encodes the bytes of a single TOC Track Descriptor as specified in Table 236 of [MMC-3](https://www.13thmonkey.org/documentation/SCSI/mmc3r10g.pdf).

```c++
struct PointTableEntry {
	byte_t entry[11];
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
	ui08_t : 8;
};
```

## Compression methods

The compression methods detailed below may employ bitstream packing. In such cases, the bytes are processed in sequence with bits processed from the most significant bit to the least significant bit.

### RUN_LENGTH_ENCODING

The RUN_LENGTH_ENCODING compression method employs a simple mechanism for encoding sectors containing sparse data, with minimal overhead.

The bitstream consists of one or more sequences for which the first bit indicates the type of sequence. A zero bit indicates a raw sequence whereas a one bit indicates a run sequence.

An x-value is encoded imediately following the type bit. The x-value is encoded using [Exponential Golomb coding](https://en.wikipedia.org/wiki/Exponential-Golomb_coding) with parameter k set to 0 for which the first eight x-values are encoded as follows.

| X-value | Bits    |
| ------- | ------- |
| 0       | 1       |
| 1       | 010     |
| 2       | 011     |
| 3       | 00100   |
| 4       | 00101   |
| 5       | 00110   |
| 6       | 00111   |
| 7       | 0001000 |

For raw sequences, the byte length is 1 more than the encoded x-value. Immediately following the x-value are `x + 1` 8-bit octets.

For run sequences, the byte length is 2 more than the encoded x-value. Immediately following is a single 8-bit octet. The octet is to be repeated `x + 2` times.

The bit stream ``0,010,00000001,00000010,1,011,00000011` encodes the byte sequence `0x01 0x02 0x03 0x03 0x03 0x03`. The encoded bitstream is packed into octets as `00100000,00010000,00101011,00000011`. The compression ratio becomes 4/6.

Empty sector data is encoded using a total of 4 bytes using the RUN_LENGTH_ENCODING method resulting in a compression ratio of 4/2352. Empty subchannels data is encoded using a total of 3 bytes resulting in a compression ratio of 3/96.

### LOSSLESS_STEREO_AUDIO

The LOSSLESS_STEREO_AUDIO compression method employs a mechanism for encoding sector data containing audio signals. The method is very similar to other lossless audio compression methods such as [FLAC](https://en.wikipedia.org/wiki/FLAC).

The left and right channels are spatially decorrelated by subtracting the left channel from the right channel. The channels are encoded in sequence with left, right order.

Each channel uses one of four predictors for temporal decorrelation. The predictors are `NONE (0)`, `CONSTANT_EXTRAPOLATION (1)`, `LINEAR_EXRAPOLATION (2)` and `QUADRATIC_EXTRAPOLATION (3)`. Each predictor uses between one and three of the previous samples to predict the next sample for the channel in question using a linear equation. The first sample of each channel uses the `NONE (0)` predictor and is also used to extrapolate the signal before its start.

| Predictor               | Prediction                                  |
| ----------------------- | ------------------------------------------- |
| NONE                    | p(t) = 0 * p(t-1) + 0 * p(t-2) + 0 * p(t-3) |
| CONSTANT_EXTRAPOLATION  | p(t) = 1 * p(t-1) + 0 * p(t-2) + 0 * p(t-3) |
| LINEAR_EXRAPOLATION     | p(t) = 2 * p(t-1) - 1 * p(t-2) + 0 * p(t-3) |
| QUADRATIC_EXTRAPOLATION | p(t) = 3 * p(t-1) - 3 * p(t-2) + 1 * p(t-3) |

The choice of predictor is stored using two bits.

## Sponsorship

The continued development of this software depends on your sponsorship. Please consider sponsoring this project if you find that the software creates value for you and your organization.

The sponsor button can be used to view the different sponsoring options. Contributions of all sizes are welcome.

Thank you for your support!

### Ethereum

Ethereum contributions can be made to address `0xf1B63d95BEfEdAf70B3623B1A4Ba0D9CE7F2fE6D`.

![](./eth.png)

## Roadmap

* Investigate possibility of disabling read cache.
	Can be done by issuing normal READ SCSI command but might not be respected by the drive.
* Implement C2-based refinement.
	Transfer block setting is controlled through ReadWriteErrorRecoveryModePage.
	Disable RSPC for data tracks.
	Set error recovery to min or max.
	Read track in several passes and refine sector data using C2 information for all track types.
	Apply software RSPC for data tracks.
* Add support for injecting files into ISO9660 file systems.
	Requires software RSPC generation for raw image formats.
* Reorganize exceptions so that enums and types can be re-used.
* Improve copier to better extract subchannels.
* Detect indices in audio tracks including index 00 in pregap.
* Document utility.
* Document ODI format.

## References

* [Seagate SCSI Reference J](https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf)
* [NCITS SCSI Multi Media Commands 3 Reference](https://www.13thmonkey.org/documentation/SCSI/mmc3r10g.pdf)
* [INCITS SCSI Block Commands 3 Reference](https://www.t10.org/ftp/t10/document.05/05-344r0.pdf)
* [INCITS SCSI Primary Commands 4 Reference](https://dn790004.ca.archive.org/0/items/SCSISpecificationDocumentsSCSIDocuments/SCSI%20Primary%20Commands/SCSI%20Primary%20Commands%204%20rev%2016.pdf)

## Different CD-ROM formats

### Mixed-mode CD-ROM

The mixed-mode CD-ROM format is used to store tracks with mixed content types in a single session. The format pre-dates the multi-session CD-ROM format and was commonly used by video games released around the turn of the century.

The first track of a mixed-mode CD-ROM contains data and all subsequent tracks contain audio.

```
[Session 1 Lead-In] (4500 sectors)
[Session 1 Track 1 Pregap] (150 sectors)
[Session 1 Track 1 Data]
[Session 1 Track 1 Postgap] (150 sectors) (required due to coming change in track type)
[Session 1 Track 2 Pregap] (optional)
[Session 1 Track 2 Audio]
[Session 1 Track 3 Pregap] (optional)
[Session 1 Track 3 Audio]
[Session 1 Lead-Out] (6750 sectors)
```

### Multi-session CD-ROM

The multi-session CD-ROM format is used to store tracks with mixed content types in multiple sessions. Each session may in turn contain mixed-mode content.

```
[Session 1 Lead-In] (4500 sectors)
[Session 1 Track 1 Pregap] (at least 150 sectors)
[Session 1 Track 1 Audio]
[Session 1 Track 2 Pregap] (optional)
[Session 1 Track 2 Audio]
[Session 1 Lead-Out] (6750 sectors)

[Session 2 Lead-In] (4500 sectors)
[Session 2 Track 3 Pregap] (150 sectors)
[Session 2 Track 3 Data]
[Session 2 Lead-Out] (2250 sectors)

[Session 3 Lead-In] (4500 sectors)
[Session 3 Track 4 Pregap] (150 sectors)
[Session 3 Track 4 Data]
[Session 3 Lead-Out] (2250 sectors)
```

## Read correction

Audio tracks on CD-DA and mixed-mode CD-ROMs should be extracted using read offset correction. The operation requires knowledge about the `read offset value` of the optical drive used. The value can be either negative or positive where a negative value indicates that the drive in question starts reading audio streams too early. Conversely, a positive value indicates that the drive in question starts reading audio streams too late.

A track with the contents "TRACK" read through a drive with a negative offset of one letter would read the contents as "-TRAC" whereas a drive with a positive offset of one letter would read the contents as "RACK-". In the negative case, the data read includes data not belonging to the track at the start (indicated by the leading dash) and is missing data belonging to the track at the end. In the positive case, the data read is missing data belonging to the track at the start and includes data not belonging to the track at the end (indicated by the trailing dash).

Every sector of a CD has a length of 2352 bytes which for CD-DA sectors are used to store 98 audio frames, each containing 6 stereo samples with a precision of 16-bits. A 16-bit stereo sample requires a total of 4 bytes of storage.

In order to correctly read audio streams, read offset correction has to be performed. The proper `read offset correction value` for an optical drive is the negative value of its `read offset value`. A positive `read offset correction value` should be used for a drive with a negative `read offset value`. Conversely, a negative `read offset correction value` should be used for a drive with a positive `read offset value`.

The [AccurateRip drive database](https://www.accuraterip.com/driveoffsets.htm) provides a list of correction values for a large number of drives. Positive values indicate the number of samples that should be discarded at the beginning of each track and the number of samples that should be included from the following track. Negative values indicate the number of samples that should be discarded at the end of each track and the number of samples that should be included from the previous track.
