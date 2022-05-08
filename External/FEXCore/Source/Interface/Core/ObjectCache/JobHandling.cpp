#include "Interface/Context/Context.h"
#include "Interface/Core/ObjectCache/ObjectCacheService.h"

#include <FEXCore/Config/Config.h>

#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <string>
#include <sys/uio.h>
#include <sys/mman.h>
#include <xxhash.h>

namespace FEXCore::CodeSerialize {
  void AsyncJobHandler::AsyncAddNamedRegionJob(uintptr_t Base, uintptr_t Size, uintptr_t Offset, const std::string &filename) {
    // XXX: Actually add the add named region job
  }

  void AsyncJobHandler::AsyncRemoveNamedRegionJob(uintptr_t Base, uintptr_t Size) {
    // XXX: Actually add the remove named region job
  }

  void AsyncJobHandler::AsyncAddSerializationJob(std::unique_ptr<SerializationJobData> Data) {
    // XXX: Actually add serialization job
  }
}
