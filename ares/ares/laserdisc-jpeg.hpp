#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <turbojpeg.h>

namespace ares {

class JpegVideo {
public:
  static constexpr size_t width = 910;
  static constexpr size_t height = 525;
  static constexpr size_t rgbSize = width * height * 3;

  auto open(std::span<const uint8_t> stream) -> bool {
    *this = {};
    if(stream.size() < headerSize || std::memcmp(stream.data(), "JVC1", 4) != 0) return false;
    if(read(stream.data() + 4, 2) != 1 || read(stream.data() + 6, 2) != headerSize) return false;
    if(read(stream.data() + 8, 4) != width || read(stream.data() + 12, 4) != height) return false;
    const auto count = read(stream.data() + 16, 4);
    if(!count || read(stream.data() + 20, 4) != 1 || read(stream.data() + 24, 8) != headerSize) return false;
    if(count > (stream.size() - headerSize) / entrySize) return false;
    auto offset = headerSize + count * entrySize;
    if(read(stream.data() + 32, 8) != offset) return false;
    for(size_t index = 0; index < count; ++index) {
      const auto entry = stream.data() + headerSize + index * entrySize;
      const auto size = read(entry + 8, 4);
      if(read(entry, 8) != offset || size < 4 || size > stream.size() - offset || read(entry + 12, 4) != index) return false;
      offset += size;
    }
    if(offset != stream.size()) return false;
    data = stream;
    frameCount = count;
    return true;
  }

  auto frame(size_t index) const -> const uint8_t* {
    return index < frameCount ? data.data() + headerSize + index * entrySize : nullptr;
  }

  template<typename Frames>
  auto indexRegion(Frames& frames, size_t requested, size_t& offset) const -> void {
    const size_t count = std::min(requested, frameCount - offset);
    frames.clear();
    frames.reserve(count);
    for(size_t index = 0; index < count; ++index) frames.push_back(frame(offset++));
  }

  auto packet(const uint8_t* entry) const -> std::span<const uint8_t> {
    return data.subspan(read(entry, 8), read(entry + 8, 4));
  }

  auto code(size_t index, int line) const -> uint32_t {
    auto entry = frame(index);
    if(!entry) return 0;
    switch(line) {
    case 15: return read(entry + 16, 4);
    case 16: return read(entry + 20, 4);
    case 278: return read(entry + 24, 4);
    case 279: return read(entry + 28, 4);
    default: return 0;
    }
  }

  size_t frameCount = 0;

private:
  static constexpr size_t headerSize = 64;
  static constexpr size_t entrySize = 32;

  static auto read(const uint8_t* bytes, size_t count) -> uint64_t {
    uint64_t value = 0;
    for(size_t index = count; index-- > 0;) value = (value << 8) | bytes[index];
    return value;
  }

  std::span<const uint8_t> data;
};

class JpegDecoder {
public:
  JpegDecoder() = default;
  JpegDecoder(const JpegDecoder&) = delete;
  auto operator=(const JpegDecoder&) -> JpegDecoder& = delete;
  ~JpegDecoder() { reset(); }

  auto reset() -> void {
    if(codec) tj3Destroy(codec);
    codec = nullptr;
  }

  auto decode(std::span<const uint8_t> jpeg, std::span<uint8_t> rgb) -> bool {
    if(rgb.size() != JpegVideo::rgbSize) return false;
    if(!codec) {
      codec = tj3Init(TJINIT_DECOMPRESS);
      if(!codec) return false;
      if(tj3Set(codec, TJPARAM_MAXPIXELS, JpegVideo::width * JpegVideo::height) ||
          tj3Set(codec, TJPARAM_STOPONWARNING, 1)) {
        reset();
        return false;
      }
    }
    if(tj3DecompressHeader(codec, jpeg.data(), jpeg.size()) ||
        tj3Get(codec, TJPARAM_JPEGWIDTH) != JpegVideo::width ||
        tj3Get(codec, TJPARAM_JPEGHEIGHT) != JpegVideo::height ||
        tj3Get(codec, TJPARAM_PRECISION) != 8 ||
        tj3Get(codec, TJPARAM_SUBSAMP) != TJSAMP_444 ||
        tj3Get(codec, TJPARAM_PROGRESSIVE) != 0) return false;
    return tj3Decompress8(codec, jpeg.data(), jpeg.size(), rgb.data(), JpegVideo::width * 3, TJPF_RGB) == 0;
  }

private:
  tjhandle codec = nullptr;
};

}
