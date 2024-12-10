# The ODI file format

## Versioning

The Overdrive image file format is semantically versioned, implying that breaking changes trigger a new major version while backward-compatible changes trigger a new minor version. This specification details the 1.0 version of the format.

## Endianness

Little-endian data types are exclusively used within the format meaning that the 32-bit value `0x12345678` is encoded as the byte sequence `0x78 0x56 0x34 0x12`.

## Offsets

All offsets are specified as absolute offsets (i.e. relative to the beginining of the image file). They are currently defined to be 32-bit wide implying a maximum file size of 4 GB. However, additional space is reserved in the format allowing for all offsets to be extended up to a width of at most 48 bits. Implementations should be able to handle 48-bit wide offsets.

## Sector range

The format stores an image of an optical disc starting at the first sector of the lead-in area of the first session, i.e. sector -4500. The format stores all sectors until the last sector of the lead-out area of the last session. The lead-out area of the last session is 6750 sectors long for single-session discs and 2250 sectors long for multi-session discs.

## Structure

### The file header

The disc image is stored in a single file which starts with a `FileHeader` structure. The structure contains a `header_length` member, indicating the total number of bytes occupied by the `FileHeader` structure. The length is currently 16 bytes but the value may increase in future versions of the format. Implementations are expected to be able to handle longer lengths transparently.

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

The `sector_table_header_absolute_offset` member specifies the absolute offset of the `SectorTableHeader` structure and the `point_table_header_absolute_offset` member specifies the absolute offset of the `PointTableHeader` structure. The sector table contains information about the sectors stored in the image while the point table contains information about the points stored in the image. The points can be used to derive information about the sessions and tracks of the optical disc used to create the image.

### The sector table

The `SectorTableHeader` structure contains three fields which are all required in order to read entries from the sector table.

The structure contains a `header_length` member, indicating the total number of bytes occupied by the `SectorTableHeader` structure. The structure also contains an `entry_length` member, indicating the total number of bytes occupied by each of the `SectorTableEntry` structures. Both lengths are currently 16 bytes but the values may increase in future versions of the format. Implementations are expected to be able to handle longer lengths transparently.

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

Each `SectorTableEntry` structure provides information about a single sector. The `compressed_data_absolute_offset` member specifies the absolute offset of the compressed data. All compressed data is stored concatenated at the offset in question. Sector data is followed by subchannels data without any padding. The subchannels are stored deinterleaved in P to W order.

The `readability` member specifies whether or not the sector was successfully read from the optical disc. Valid options are `UNREADABLE (0x00)` and `READABLE (0x01)`. Implementations are expected to treat other values as an error.

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

Valid options for the `compression_method` member are `NONE (0x00)`, `RUN_LENGTH_ENCODING (0x01)` and `LOSSLESS_STEREO_AUDIO (0x80)`. Implementations are expected to treat other values as an error.

The compression method `NONE (0x00)` indicates that the data is stored uncompressed and that `compressed_byte_count` bytes can be read directly. The other compression algorithms are detailed in the `Compression methods` section of this specification. The compressed byte count is always less than or equal to 2352 bytes.

```c++
struct SectorDataCompressionHeader {
	SectorDataCompressionMethod::type compression_method;
	ui16_t compressed_byte_count;
};
```

The `subchannels_data` member contains an embedded `SubchannelsDataCompressionHeader` structure whose members `compression_method` and `compressed_byte_count` specify the compression method used to store the subchannels data and the resulting compressed byte count.

Valid options for the `compression_method` member are `NONE (0x00)` and `RUN_LENGTH_ENCODING (0x01)`. Implementations are expected to treat other values as an error.

The compression method `NONE (0x00)` indicates that the data is stored uncompressed and that `compressed_byte_count` bytes can be read directly. The other compression algorithms are detailed in the `Compression methods` section of this specification. The compressed byte count is always less than or equal to 96 bytes.

```c++
struct SubchannelsDataCompressionHeader {
	SubchannelsDataCompressionMethod::type compression_method;
	ui16_t compressed_byte_count;
};
```

### The point table

The `PointTableHeader` structure contains three fields which are all required in order to read entries from the point table.

The structure contains a `header_length` member, indicating the total number of bytes occupied by the `PointTableHeader` structure. The structure also contains an `entry_length` member, indicating the total number of bytes occupied by each of the `PointTableEntry` structures. Both lengths are currently 16 bytes but the values may increase in future versions of the format. Implementations are expected to be able to handle longer lengths transparently.

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

Each `PointTableEntry` structure provides information about a single point. The `descriptor` member encodes the bytes of a single `TOC Track Descriptor` as specified in Table 236 of [MMC-3](https://www.13thmonkey.org/documentation/SCSI/mmc3r10g.pdf).

```c++
struct PointTableEntry {
	byte_t descriptor[11];
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

The `RUN_LENGTH_ENCODING` compression method employs a simple mechanism for encoding sectors containing sparse data, creating minimal overhead.

The bitstream consists of one or more sequences for which the first bit indicates the type of sequence. A zero bit indicates a raw sequence whereas a one bit indicates a run sequence.

An `x-value` is encoded imediately following the type bit. The `x-value` is encoded using [Exponential Golomb coding](https://en.wikipedia.org/wiki/Exponential-Golomb_coding) with the k parameter set to `0` for which the first eight `x-values` are encoded as follows.

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

For raw sequences, the byte length is 1 more than the encoded `x-value`. Immediately following the `x-value` are `x + 1` 8-bit octets.

For run sequences, the byte length is 2 more than the encoded `x-value`. Immediately following is a single 8-bit octet. The octet is to be repeated `x + 2` times.

The byte sequence `0x01 0x02 0x03 0x03 0x03 0x03` is encoded using one raw sequence followed by a run sequence as the bitstream `(0,010,00000001,00000010),(1,011,00000011)`. The encoded bitstream is packed into complete octets as `00100000,00010000,00101011,00000011` and the compression ratio becomes 4/6.

Empty sector data is encoded using a total of 4 bytes using the `RUN_LENGTH_ENCODING` method resulting in a compression ratio of 4/2352. Empty subchannels data is encoded using a total of 3 bytes resulting in a compression ratio of 3/96.

### LOSSLESS_STEREO_AUDIO

The `LOSSLESS_STEREO_AUDIO` compression method employs a mechanism for encoding sector data containing audio signals. The method is very similar to other lossless audio compression methods such as [FLAC](https://en.wikipedia.org/wiki/FLAC).

The left and right channels are spatially decorrelated by subtracting the left channel from the right channel. The channels are encoded independently in left, right order.

Each channel uses one of four predictors for temporal decorrelation. The predictors are `NONE (0)`, `CONSTANT_EXTRAPOLATION (1)`, `LINEAR_EXTRAPOLATION (2)` and `QUADRATIC_EXTRAPOLATION (3)`. Each predictor may use the previous three samples to predict the next sample for the channel in question through a linear equation. The first sample of each channel always uses the `NONE (0)` predictor and is also used to extrapolate the signal before its start.

| Predictor               | Prediction                                  |
| ----------------------- | ------------------------------------------- |
| NONE                    | p(t) = 0 * p(t-1) + 0 * p(t-2) + 0 * p(t-3) |
| CONSTANT_EXTRAPOLATION  | p(t) = 1 * p(t-1) + 0 * p(t-2) + 0 * p(t-3) |
| LINEAR_EXTRAPOLATION     | p(t) = 2 * p(t-1) - 1 * p(t-2) + 0 * p(t-3) |
| QUADRATIC_EXTRAPOLATION | p(t) = 3 * p(t-1) - 3 * p(t-2) + 1 * p(t-3) |

The choice of predictor is encoded using two bits.

The signed residuals are transformed using the transform below. The transform minimizes the magnitudes of the unsigned values, reducing the number of bits required for coding each value in the coding stage.

```c++
auto unsigned_value = signed_value < 0 ? 0 - (signed_value << 1) - 1 : signed_value << 1;
auto signed_value = (unsigned_value & 1) ? 0 - ((unsigned_value + 1) >> 1) : unsigned_value >> 1;
```

The unsigned residuals are encoded using [Rice coding](https://en.wikipedia.org/wiki/Rice_coding) for which the `Rice parameter` is is selected in the range 0-15 such that the encoding uses the fewest bits possible.

The choice of `Rice parameter` is encoded using four bits.

The 16-bit sample sequences `0x0000 0x0001 0x0002` and `0x0000 0x0001 0x0002` for the left and right channels are encoded using the `LINEAR_EXTRAPOLATION (2)` predictor and using `Rice parameter` 0 as the bitstream `(10,0000,10,0010,000010),(10,0000,10,10,10)`. The encoded bitstream is packed into complete octets as `10000010,00100000,10100000,10101000` and the compression ratio becomes 4/12.

The `LOSSLESS_STEREO_AUDIO` compression method usually yields compression ratios between 70% and 80%.
