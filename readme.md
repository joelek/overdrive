# Overdrive

Utility for interfacing with optical drives. Written for the Windows platform.

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

## Roadmap

* Investigate possibility of disabling read cache.
	Can be done by issuing normal READ SCSI command but might not be respected by the drive.
* Implement C2-based refinement.
	Transfer block setting is controlled through ReadWriteErrorRecoveryModePage.
	Disable RSPC for data tracks.
	Set error recovery to min or max.
	Read track in several passes and refine sector data using C2 information for all track types.
	Apply software RSPC for data tracks.
* Add debug logging.
* Add support for injecting files into ISO9660 file systems.
	Requires software RSPC generation for raw image formats.
* Reorganize exceptions so that enums and types can be re-used.
* Improve copier to better extract subchannels.
* Detect indices in audio tracks including index 00 in pregap.
* Investigate possibility of reading lead-in and lead-out when cdrom error correction is turned off.
* Optimize commands to not store 150 sectors of silence between data and audio tracks on mixed-mode CD-ROMs.
* Change all enums to namespaces.
* Add sense pointer to ioctl().
* Rename repository.
* Document utility.
* Add compression to ODI format.

## References

* [Seagate SCSI Reference J](https://www.seagate.com/files/staticfiles/support/docs/manual/Interface%20manuals/100293068j.pdf)
* [NCITS SCSI Multi Media Commands 3 Reference](https://www.13thmonkey.org/documentation/SCSI/mmc3r10g.pdf)
* [INCITS SCSI Block Commands 3 Reference](https://www.t10.org/ftp/t10/document.05/05-344r0.pdf)
* [INCITS SCSI Primary Commands 4 Reference](https://dn790004.ca.archive.org/0/items/SCSISpecificationDocumentsSCSIDocuments/SCSI%20Primary%20Commands/SCSI%20Primary%20Commands%204%20rev%2016.pdf)
