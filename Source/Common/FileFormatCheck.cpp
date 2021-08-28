#include <fstream>
#include <stdint.h>
#include <string>

namespace FEX::FormatCheck {
  bool IsSquashFS(std::string const &Filename) {
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
    std::fstream File(Filename, std::ios::in);

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
}
