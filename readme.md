# Overdrive

Low-level utility for creating archival disc images from data, audio and mixed-mode optical discs.

```
overdrive cue F: image.cue
```

## Tasks

Overdrive can be used to create optical disc images in several different file formats through its various tasks. Each task comes with its own set of options which may be displayed through the `overdrive <task>` command.

### BIN/CUE

Create a disc image with the BIN/CUE file format through the `cue` task. The format is recommended for basic discs containing audio tracks.

```
overdrive cue F: image.cue
```

### ISO

Create a disc image with the ISO file format through the `iso` task. The format is recommended for basic discs containing a single data track.

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

Overdrive includes support for mounting ODI files which is activated by specifying the path of an ODI file as the source drive. This provides a convenient way of converting ODI files into any of the other file formats supported by Overdrive.

```
overdrive cue image-to-be-mounted.odi image-to-be-written.cue
```

## The ODI file format

The [ODI file format](./odi.md) is Overdrive's own compact, highly-flexible and freely available disc image format. A [reference implementation](./source/lib/odi.h) is available in Overdrive's source code.

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
* Detect indices in audio tracks including index 00 in pregap.

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
