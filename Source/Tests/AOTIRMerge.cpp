/*
$info$
tags: Bin|AOTIRMerge
desc: Used merge AOTIR cache
$end_info$
*/

#include <FEXCore/Config/Config.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/IR/AOTIR.h>

#include <atomic>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

using namespace FEXCore::IR;

static std::string AOTDir;
static std::map<std::string, std::vector<std::string>> AOTFileMap;
std::atomic<int> PIDCounter = 1;
static std::set<uint64_t> PIDSet;

static inline std::string getfileid(const std::string& filename) {
    return std::string(filename, 0, filename.find(".aotir"));
}

static inline std::string getmodename(const std::string& filename) {
    auto pos = filename.find_last_of('-');
    return std::string(filename, 0, filename.find_last_of('-', pos - 1));
}

static bool readAll(int fd, void *data, size_t size) {
  int rv = read(fd, data, size);

  if (rv != size)
    return false;
  else
    return true;
}

static bool MapFile(int streamfd, AOTIRCacheEntry *AOTIRCache) {
    uint64_t tag;
    if (!readAll(streamfd, (char*)&tag, sizeof(tag)) || tag != AOTIR_COOKIE)
      return false;

    lseek(streamfd, -sizeof(tag), SEEK_END);
    if (!readAll(streamfd, (char*)&tag, sizeof(tag)) || tag != AOTIR_COOKIE)
      return false;

    std::string Module;
    uint64_t ModSize;
    uint64_t IndexSize;

    lseek(streamfd, -sizeof(ModSize) - sizeof(tag), SEEK_END);

    if (!readAll(streamfd,  (char*)&ModSize, sizeof(ModSize)))
      return false;

    Module.resize(ModSize);

    lseek(streamfd, -sizeof(ModSize) - ModSize - sizeof(tag), SEEK_END);

    if (!readAll(streamfd,  (char*)&Module[0], Module.size()))
      return false;

    lseek(streamfd, -sizeof(ModSize) - ModSize - sizeof(IndexSize) - sizeof(tag), SEEK_END);

    if (!readAll(streamfd,  (char*)&IndexSize, sizeof(IndexSize)))
      return false;

    struct stat fileinfo;
    if (fstat(streamfd, &fileinfo) < 0)
      return false;
    size_t Size = (fileinfo.st_size + 4095) & ~4095;
    size_t IndexOffset = fileinfo.st_size - IndexSize -sizeof(ModSize) - ModSize - sizeof(IndexSize) - sizeof(tag);

    void *FilePtr = FEXCore::Allocator::mmap(nullptr, Size, PROT_READ, MAP_SHARED, streamfd, 0);

    if (FilePtr == MAP_FAILED) {
      return false;
    }

    auto Array = (AOTIRInlineIndex *)((char*)FilePtr + IndexOffset);
    AOTIRCache->Array = Array;
    AOTIRCache->mapping = FilePtr;
    AOTIRCache->size = Size;
    return true;
}

void LoadAOTTempFile(int tmpfd, std::ofstream& finalfile, std::multimap<uint64_t, uint64_t>& index, std::set<uint64_t>& hashSet) {
    AOTIRCacheEntry aotcache;
    if (!MapFile(tmpfd, &aotcache)) return ;
    AOTIRInlineIndex *Array = aotcache.Array;
    for (auto i = 0; i < Array->Count; i++) {
        AOTIRInlineEntry* entry = Array->GetInlineEntry(Array->Entries[i].DataOffset);
        if (hashSet.contains(entry->GuestHash)) continue;
        hashSet.insert(entry->GuestHash);
        index.insert({Array->Entries[i].GuestStart, finalfile.tellp()});
        finalfile.write((char*)&entry->GuestHash, sizeof(entry->GuestHash));
        finalfile.write((char*)&entry->GuestLength, sizeof(entry->GuestLength));
        entry->GetRAData()->Serialize(finalfile);
        entry->GetIRData()->Serialize(finalfile);
    }
}

void Merge(std::vector<std::string>& files) {
    if (files.empty()) {
      return;
    }
    auto fileid = getfileid(files[0]);
    auto filepath = std::filesystem::path(AOTDir) / (fileid + ".aotir");

    if (std::filesystem::exists(filepath)) {
        std::filesystem::rename(filepath, std::filesystem::path(AOTDir) / (fileid + ".aotir.0000.tmp"));
        files.push_back(fileid + ".aotir.0000.tmp");
    }
    std::ofstream finalfile(filepath, std::ios::out | std::ios::binary);

    std::multimap<uint64_t, uint64_t> index; // [GuestStart, DataOffset]
    std::set<uint64_t> hashSet;
    auto tag = AOTIR_COOKIE;
    finalfile.write((char*)&tag, sizeof(tag));
    for (auto& file : files) {
        std::string fullname{AOTDir + "/" + file};
        int fd = open(fullname.c_str(), O_RDONLY);
        if (fd > 0) { 
            LoadAOTTempFile(fd, finalfile, index, hashSet);
            close(fd);
        }
        remove(fullname.c_str());
    }
    // pad to 32 bytes
    constexpr char Zero = 0;
    while(finalfile.tellp() & 31)
        finalfile.write(&Zero, 1);

    // AOTIRInlineIndex
    const auto FnCount = index.size();
    const size_t DataBase = -finalfile.tellp();

    finalfile.write((const char*)&FnCount, sizeof(FnCount));
    finalfile.write((const char*)&DataBase, sizeof(DataBase));
    for (const auto& [GuestStart, DataOffset] : index) {
        //AOTIRInlineIndexEntry
        // GuestStart
        finalfile.write((const char*)&GuestStart, sizeof(GuestStart));
        // DataOffset
        finalfile.write((const char*)&DataOffset, sizeof(DataOffset));
    }
    // End of file header
    uint64_t ModSize = fileid.size();
    const auto IndexSize = FnCount * sizeof(AOTIRInlineIndexEntry) + sizeof(DataBase) + sizeof(FnCount);
    finalfile.write((const char*)&IndexSize, sizeof(IndexSize));
    finalfile.write(fileid.c_str(), ModSize);
    finalfile.write((const char*)&ModSize, sizeof(ModSize));
    finalfile.write((char*)&tag, sizeof(tag));
    finalfile.close();
}


void AOTIRMerge() {
    if (!std::filesystem::is_directory(AOTDir)) {
        fmt::print(stderr, "AOTIRMerge: {} is not directory\n", AOTDir);
        return ;
    }
    for (auto& entry : std::filesystem::directory_iterator(AOTDir)) {
        auto filename = entry.path().filename();
        if (filename.extension().string() != ".tmp") continue;
        auto modename = getmodename(filename.string());
        AOTFileMap[modename].push_back(filename.string());
    }
    for (auto& [_, files] : AOTFileMap) {
        Merge(files);       
    }
}

void SignalHandler(int signo, siginfo_t* siginfo, void* data) {
  switch (signo) {
  case SIGUSR1:
    if (!PIDSet.contains(siginfo->si_pid)) {
      PIDSet.insert(siginfo->si_pid);
      PIDCounter++;
    }
    break;
  case SIGUSR2:
    if (PIDSet.contains(siginfo->si_pid)) {
      PIDSet.erase(siginfo->si_pid);
      PIDCounter--;
    }
    break;
  default:
    break;
  }
}

int main() {
  FEXCore::Config::Initialize();
  FEXCore::Config::Load();
  FEX_CONFIG_OPT(AOTIRMergePID, AOTIRMERGEPIDFILE);
  PIDSet.insert(getppid());
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = SignalHandler;
  if (sigaction(SIGUSR1, &sa, NULL) == -1) {
    fmt::print(stderr, "AOTIRMerge: Set SIGUSR1 handler failed\n");
    exit(0);
  }
  if (sigaction(SIGUSR2, &sa, NULL) == -1) {
    fmt::print(stderr, "AOTIRMerge: Set SIGUSR2 handler failed\n");
    exit(0);
  }
  while (PIDCounter) {
    sleep(5);
    for (auto pid : PIDSet) {
      // some guest process may exit abnormal, waiting is not necessary.
      auto piddir = "/proc/" + std::to_string(pid);
      if (!std::filesystem::exists(piddir)) {
        PIDSet.erase(pid);
        PIDCounter--;
      }
    }
  }
  AOTDir = FEXCore::Config::GetDataDirectory() + "aotir";
  AOTIRMerge();
  if (std::filesystem::exists(AOTIRMergePID())) {
    std::filesystem::remove(AOTIRMergePID());
  }
  return 0;
}