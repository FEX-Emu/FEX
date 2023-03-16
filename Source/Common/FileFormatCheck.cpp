#include <FEXCore/fextl/string.h>

#include <fstream>
#include <stdint.h>

namespace FEX::FormatCheck {
  bool IsSquashFS(fextl::string const &Filename) {
    // If it is a regular file then we need to check if it is a valid archive
    struct SquashFSHeader {
      uint32_t magic;
      uint32_t inode_count;
      uint32_t mtime;
      uint32_t block_size;
      uint32_t fragment_entry_count;
      uint16_t compression_id;
      uint16_t block_log;
      uint16_t flags;
      uint16_t id_count;
      uint16_t version_major;
      uint16_t version_minor;
      uint64_t More[8]; // More things that don't matter to us
    };

    SquashFSHeader Header{};
    std::fstream File(Filename.c_str(), std::ios::in);

    if (!File.is_open()) {
      return false;
    }

    if (!File.seekg(0, std::fstream::end)) {
      return false;
    }

    auto FileSize = File.tellg();
    if (File.fail()) {
      return false;
    }

    if (FileSize <= 0) {
      return false;
    }

    if (!File.seekg(0, std::fstream::beg)) {
      return false;
    }

    if (FileSize < sizeof(SquashFSHeader)) {
      return false;
    }

    if (!File.read(reinterpret_cast<char*>(&Header), sizeof(SquashFSHeader))) {
      return false;
    }

    // Make sure the cookie matches
    if (Header.magic == 0x73717368) {
      // Sanity check the version
      uint32_t version = (uint32_t)Header.version_major << 16 | Header.version_minor;
      if (version >= 0x00040000) {
        // Everything is sane, we can add it
        return true;
      }
    }
    return false;
  }

  bool IsEroFS(fextl::string const &Filename) {
    // v1 of EroFS has a 128byte header
    // This lives within a fixed offset inside of the first superblock of the file
    // Each superblock is 4096bytes
    //
    // We only care about the uint32_t at the start of this offset which is the cookie
    struct EroFSHeader {
      uint32_t Magic;
      // Additional data after this if necessary in the future.
    };

    constexpr size_t HEADER_OFFSET = 1024;
    constexpr uint32_t COOKIE_MAGIC_V1 = 0xE0F5E1E2;

    EroFSHeader Header{};
    std::fstream File(Filename.c_str(), std::ios::in);

    if (!File.is_open()) {
      return false;
    }

    if (!File.seekg(HEADER_OFFSET, std::fstream::beg)) {
      return false;
    }

    if (!File.read(reinterpret_cast<char*>(&Header), sizeof(EroFSHeader))) {
      return false;
    }

    return Header.Magic == COOKIE_MAGIC_V1;
  }
}
