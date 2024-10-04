# Overdrive

Utility for interfacing with optical drives. Written for the Windows platform.

## Roadmap

* Investigate possibility of disabling read cache.
	Can be done by issuing normal READ SCSI command.
* Decide on strategy for storing unreadable sectors.
* Implement C2-based refinement.
* Handle read errors during audio track extraction.
* Add separate retry count for audio tracks.
* Add ignore read error setting (data tracks only).
* Prevent reading into end of data tracks when read offset correction is negative.
* Add support for custom user data sizes in iso9660 module.
* Make utility platform-independent.
* Add debug logging.
* Show __FILE__ and __LINE__ in exception messages for DEBUG mode.
* Move hexdump to type unit.
* Investigate using namespace with constants over enums.
* Add disc and session sector lengths.

## Read offset correction

Audio tracks on CD-DA and mixed mode CD-ROMs should be extracted using read offset correction. The operation requires knowledge about the `read offset value` of the optical drive used. The value can be either negative or positive where a negative value indicates that the drive in question starts reading audio streams too early. Conversely, a positive value indicates that the drive in question starts reading audio streams too late.

A track with the contents "TRACK" read through a drive with a negative offset of one letter would read the contents as "-TRAC" whereas a drive with a positive offset of one letter would read the contents as "RACK-". In the negative case, the data read includes data not belonging to the track at the start (indicated by the leading dash) and is missing data belonging to the track at the end. In the positive case, the data read is missing data belonging to the track at the start and includes data not belonging to the track at the end (indicated by the trailing dash).

Each CD-DA sector has a length of 2352 bytes which are used to store 98 audio frames, each containing 6 stereo samples with a precision of 16-bits. A 16-bit stereo sample requires a total of 4 bytes of storage.

In order to correctly read audio streams, read offset correction has to be performed. The proper `read offset correction value` for an optical drive is the negative value of its `read offset value`. A positive `read offset correction value` should be used for a drive with a negative `read offset value`. Conversely, a negative `read offset correction value` should be used for a drive with a positive `read offset value`.

The [AccurateRip drive database](https://www.accuraterip.com/driveoffsets.htm) provides a list of correction values for a large number of drives. Positive values indicate the number of samples that should be discarded at the beginning of each track and the number of samples that should be included from the following track. Negative values indicate the number of samples that should be discarded at the end of each track and the number of samples that should be included from the previous track.

## References

* [Seagate SCSI Reference J](https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf)
* [NCITS SCSI Multi Media Commands 3 Reference](https://www.13thmonkey.org/documentation/SCSI/mmc3r10g.pdf)
* [Seagate SCSI Reference D](https://enos.itcollege.ee/~edmund/storage/loengud/varasem/SAN_IPSAN_NAS_CAS/SCSI-command-reference-manual.pdf)
* [INCITS SCSI Block Commands 3 Reference](https://www.t10.org/ftp/t10/document.05/05-344r0.pdf)
