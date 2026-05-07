#pragma once

#include <cstdint>

namespace gf {
namespace zip {

#pragma pack(push, 1)

static constexpr uint32_t kLocalFileHeaderSig = 0x04034b50;
static constexpr uint32_t kCentralDirHeaderSig = 0x02014b50;
static constexpr uint32_t kEndOfCentralDirSig = 0x06054b50;

struct LocalFileHeader {
  uint32_t signature; // 0x04034b50
  uint16_t versionNeeded;
  uint16_t flags;
  uint16_t compression;
  uint16_t modTime;
  uint16_t modDate;
  uint32_t crc32;
  uint32_t compressedSize;
  uint32_t uncompressedSize;
  uint16_t fileNameLength;
  uint16_t extraFieldLength;
};

struct CentralDirHeader {
  uint32_t signature; // 0x02014b50
  uint16_t versionMade;
  uint16_t versionNeeded;
  uint16_t flags;
  uint16_t compression;
  uint16_t modTime;
  uint16_t modDate;
  uint32_t crc32;
  uint32_t compressedSize;
  uint32_t uncompressedSize;
  uint16_t fileNameLength;
  uint16_t extraFieldLength;
  uint16_t commentLength;
  uint16_t diskStart;
  uint16_t internalAttrs;
  uint32_t externalAttrs;
  uint32_t localHeaderOffset;
};

struct EndOfCentralDir {
  uint32_t signature; // 0x06054b50
  uint16_t diskNumber;
  uint16_t diskWithCentralDir;
  uint16_t numEntriesThisDisk;
  uint16_t numEntriesTotal;
  uint32_t centralDirSize;
  uint32_t centralDirOffset;
  uint16_t commentLength;
};

#pragma pack(pop)

} // namespace zip
} // namespace gf
