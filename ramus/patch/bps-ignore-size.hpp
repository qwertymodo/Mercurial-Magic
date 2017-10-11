#pragma once

#include <nall/file.hpp>
#include <nall/filemap.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>

namespace ramus {

struct bpspatch_ignore_size {
  inline auto modify(const uint8_t* data, uint size) -> bool;
  inline auto source(const uint8_t* data, uint size) -> void;
  inline auto target(uint8_t* data, uint size) -> void;

  inline auto modify(const string& filename) -> bool;
  inline auto source(const string& filename) -> bool;
  inline auto target(const string& filename) -> bool;

  inline auto metadata() const -> string;
  inline auto size() const -> uint;

  enum result : uint {
    unknown,
    success,
    patch_too_small,
    patch_invalid_header,
    source_too_small,
    target_too_small,
    patch_checksum_invalid,
  };

  inline auto apply() -> result;

protected:
  enum : uint { SourceRead, TargetRead, SourceCopy, TargetCopy };

  filemap modifyFile;
  const uint8_t* modifyData;
  uint modifySize;

  filemap sourceFile;
  const uint8_t* sourceData;
  uint sourceSize;

  filemap targetFile;
  uint8_t* targetData;
  uint targetSize;

  uint modifySourceSize;
  uint modifyTargetSize;
  uint modifyMarkupSize;
  string metadataString;
};

auto bpspatch_ignore_size::modify(const uint8_t* data, uint size) -> bool {
  if(size < 19) return false;
  modifyData = data;
  modifySize = size;

  uint offset = 4;
  auto decode = [&]() -> uint64_t {
    uint64_t data = 0, shift = 1;
    while(true) {
      uint8_t x = modifyData[offset++];
      data += (x & 0x7f) * shift;
      if(x & 0x80) break;
      shift <<= 7;
      data += shift;
    }
    return data;
  };

  modifySourceSize = decode();
  modifyTargetSize = decode();
  modifyMarkupSize = decode();

  char buffer[modifyMarkupSize + 1];
  for(uint n = 0; n < modifyMarkupSize; n++) buffer[n] = modifyData[offset++];
  buffer[modifyMarkupSize] = 0;
  metadataString = (const char*)buffer;

  return true;
}

auto bpspatch_ignore_size::source(const uint8_t* data, uint size) -> void {
  sourceData = data;
  sourceSize = size;
  modifyTargetSize = max(sourceSize, modifyTargetSize);
}

auto bpspatch_ignore_size::target(uint8_t* data, uint size) -> void {
  targetData = data;
  targetSize = size;
}

auto bpspatch_ignore_size::modify(const string& filename) -> bool {
  if(modifyFile.open(filename, filemap::mode::read) == false) return false;
  return modify(modifyFile.data(), modifyFile.size());
}

auto bpspatch_ignore_size::source(const string& filename) -> bool {
  if(sourceFile.open(filename, filemap::mode::read) == false) return false;
  source(sourceFile.data(), sourceFile.size());
  return true;
}

auto bpspatch_ignore_size::target(const string& filename) -> bool {
  file fp;
  if(fp.open(filename, file::mode::write) == false) return false;
  fp.truncate(modifyTargetSize);
  fp.close();

  if(targetFile.open(filename, filemap::mode::readwrite) == false) return false;
  target(targetFile.data(), targetFile.size());
  return true;
}

auto bpspatch_ignore_size::metadata() const -> string {
  return metadataString;
}

auto bpspatch_ignore_size::size() const -> uint {
  return modifyTargetSize;
}

auto bpspatch_ignore_size::apply() -> result {
  if(modifySize < 19) return result::patch_too_small;

  Hash::CRC32 modifyChecksum, targetChecksum;
  uint modifyOffset = 0, sourceRelativeOffset = 0, targetRelativeOffset = 0, outputOffset = 0;

  auto read = [&]() -> uint8_t {
    uint8_t data = modifyData[modifyOffset++];
    modifyChecksum.input(data);
    return data;
  };

  auto decode = [&]() -> uint64_t {
    uint64_t data = 0, shift = 1;
    while(true) {
      uint8_t x = read();
      data += (x & 0x7f) * shift;
      if(x & 0x80) break;
      shift <<= 7;
      data += shift;
    }
    return data;
  };

  auto write = [&](uint8_t data) {
    targetData[outputOffset++] = data;
    targetChecksum.input(data);
  };

  if(read() != 'B') return result::patch_invalid_header;
  if(read() != 'P') return result::patch_invalid_header;
  if(read() != 'S') return result::patch_invalid_header;
  if(read() != '1') return result::patch_invalid_header;

  modifySourceSize = decode();
  modifyTargetSize = max(modifyTargetSize, decode());
  modifyMarkupSize = decode();
  for(uint n = 0; n < modifyMarkupSize; n++) read();

  if(modifySourceSize > sourceSize) return result::source_too_small;
  if(modifyTargetSize > targetSize) return result::target_too_small;

  while(modifyOffset < modifySize - 12) {
    uint length = decode();
    uint mode = length & 3;
    length = (length >> 2) + 1;

    switch(mode) {
    case SourceRead:
      while(length--) write(sourceData[outputOffset]);
      break;
    case TargetRead:
      while(length--) write(read());
      break;
    case SourceCopy:
    case TargetCopy:
      int offset = decode();
      bool negative = offset & 1;
      offset >>= 1;
      if(negative) offset = -offset;

      if(mode == SourceCopy) {
        sourceRelativeOffset += offset;
        while(length--) write(sourceData[sourceRelativeOffset++]);
      } else {
        targetRelativeOffset += offset;
        while(length--) write(targetData[targetRelativeOffset++]);
      }
      break;
    }
  }

  uint32_t modifyModifyChecksum = 0;
  for(uint n = 0; n < 32; n += 8) read();
  for(uint n = 0; n < 32; n += 8) read();
  uint32_t checksum = modifyChecksum.digest().hex();
  for(uint n = 0; n < 32; n += 8) modifyModifyChecksum |= read() << n;

  if(checksum != modifyModifyChecksum) return result::patch_checksum_invalid;

  if(outputOffset < modifyTargetSize) {
    memory::copy(targetData + outputOffset, sourceData + outputOffset, modifyTargetSize - outputOffset);
  }

  return result::success;
}

}
