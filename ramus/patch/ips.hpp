#pragma once

#include <nall/file.hpp>
#include <nall/filemap.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>

namespace ramus {

struct ipspatch {
  inline auto modify(const uint8_t* data, uint size) -> bool;
  inline auto source(const uint8_t* data, uint size) -> void;
  inline auto target(uint8_t* data, uint size) -> void;

  inline auto modify(const string& filename) -> bool;
  inline auto source(const string& filename) -> bool;
  inline auto target(const string& filename) -> bool;

  inline auto size() const -> uint;

  enum result : uint {
    unknown,
    success,
    patch_too_small,
    patch_invalid_header,
    target_too_small,
  };

  inline auto apply() -> result;

protected:
  filemap modifyFile;
  const uint8_t* modifyData;
  uint modifySize;

  filemap sourceFile;
  const uint8_t* sourceData;
  uint sourceSize;

  filemap targetFile;
  uint8_t* targetData;
  uint targetSize;

  uint modifyTargetSize;
  bool truncate;
};

auto ipspatch::modify(const uint8_t* data, uint size) -> bool {
  if(size < 8) return false;
  modifyData = data;
  modifySize = size;

  uint offset = 5;
  auto read8 = [&]() -> uint8_t {
    uint8_t data = modifyData[offset++];
    return data;
  };
  auto read16 = [&]() -> uint16_t {
    return read8() << 8 | read8();
  };
  auto read24 = [&]() -> uint32_t {
    return read8() << 16 | read16();
  };

  uint blockAddr, blockSize, rleSize;
  uint maxBlockAddr = 0;
  modifyTargetSize = 0;
  while(offset < modifySize) {
    blockAddr = read24();
    if(blockAddr == 0x454f46) break;  //"EOF"
    maxBlockAddr = max(maxBlockAddr, blockAddr);
    blockSize = read16();
    if(blockSize == 0) {  //RLE
      rleSize = read16();
      modifyTargetSize = max(modifyTargetSize, blockAddr + rleSize);
      offset++;
    } else {
      modifyTargetSize = max(modifyTargetSize, blockAddr + blockSize);
      offset += blockSize;
    }
  }

  if(size - offset != 0 && size - offset != 3) return false;
  truncate = size - offset == 3;
  if(truncate) modifyTargetSize = read24();
  return true;
}

auto ipspatch::source(const uint8_t* data, uint size) -> void {
  sourceData = data;
  sourceSize = size;
  if(!truncate) modifyTargetSize = max(modifyTargetSize, sourceSize);
}

auto ipspatch::target(uint8_t* data, uint size) -> void {
  targetData = data;
  targetSize = size;
}

auto ipspatch::modify(const string& filename) -> bool {
  if(modifyFile.open(filename, filemap::mode::read) == false) return false;
  return modify(modifyFile.data(), modifyFile.size());
}

auto ipspatch::source(const string& filename) -> bool {
  if(sourceFile.open(filename, filemap::mode::read) == false) return false;
  source(sourceFile.data(), sourceFile.size());
  return true;
}

auto ipspatch::target(const string& filename) -> bool {
  file fp;
  if(fp.open(filename, file::mode::write) == false) return false;
  fp.truncate(modifyTargetSize);
  fp.close();

  if(targetFile.open(filename, filemap::mode::readwrite) == false) return false;
  target(targetFile.data(), targetFile.size());
  return true;
}

auto ipspatch::size() const -> uint {
  return modifyTargetSize;
}

auto ipspatch::apply() -> result {
  if(modifySize < 8) return result::patch_too_small;

  uint modifyOffset = 0, sourceRelativeOffset = 0, targetRelativeOffset = 0;

  auto read8 = [&]() -> uint8_t {
    uint8_t data = modifyData[modifyOffset++];
    return data;
  };

  auto read16 = [&]() -> uint16_t {
    return read8() << 8 | read8();
  };

  auto read24 = [&]() -> uint32_t {
    return read8() << 16 | read16();
  };

  if(read8() != 'P') return result::patch_invalid_header;
  if(read8() != 'A') return result::patch_invalid_header;
  if(read8() != 'T') return result::patch_invalid_header;
  if(read8() != 'C') return result::patch_invalid_header;
  if(read8() != 'H') return result::patch_invalid_header;

  if(modifyTargetSize > targetSize) return result::target_too_small;

  memory::copy(targetData, sourceData, sourceSize);

  uint blockAddr, blockSize, rleSize;
  while(modifyOffset < modifySize) {
    blockAddr = read24();
    if(blockAddr == 0x454f46) break;  //"EOF"
    blockSize = read16();
    if(blockSize == 0) {  //RLE
      rleSize = read16();
      memory::fill(targetData + blockAddr, rleSize, read8());
    } else {
      memory::copy(targetData + blockAddr, modifyData + modifyOffset, blockSize);
      modifyOffset += blockSize;
    }
  }

  return result::success;
}

}
