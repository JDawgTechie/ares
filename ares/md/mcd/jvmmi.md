# LaserActive Motion JPEG disc images

An `.mjmmi` is an MMI ZIP archive. The loader also accepts the prototype `.jvmmi` extension. It preserves `MediaInfo.json`, the game
tracks and the audio. For each converted side, it replaces the QON member
with a stored member at the same path with suffix `.jvcv`. The analog video
path in `MediaInfo.json` continues to point to the original QON name.

The video member begins with a 64-byte header. Integers are unsigned and
little-endian.

| Offset | Bytes | Value |
| --- | ---: | --- |
| 0 | 4 | ASCII `JVC1` |
| 4 | 2 | Version 1 |
| 6 | 2 | Header size, 64 |
| 8 | 4 | Width, 910 |
| 12 | 4 | Height, 525 |
| 16 | 4 | Frame count, including lead-in and lead-out |
| 20 | 4 | Keyframe interval, 1 |
| 24 | 8 | Index offset, 64 |
| 32 | 8 | JPEG payload offset, 64 + 32 × frame count |
| 40 | 24 | Reserved, set to zero |

The 32-byte index entries are ordered by the original QON frame number.
Each entry contains an 8-byte offset from the start of the video member,
a 4-byte JPEG packet size, a 4-byte keyframe index equal to the frame
number, and four 4-byte VBI codes. The VBI codes are taken from the
original capture rows 15, 16, 278, and 279. Zero means no valid code.

JPEG packets are contiguous and use baseline sequential 8-bit 4:4:4 JPEG.
The capture retains its two fields in the original row order, with the
second field beginning at row 263. ares decodes the requested packet
directly to RGB and uses the VBI codes from the index for disc control.

Build with `ARES_ENABLE_JVMMI=ON` and the static TurboJPEG target from
libjpeg-turbo 3. The feature supports Sega MegaLD and NEC LD-ROM² and
preserves the QON path for ordinary MMI discs.
