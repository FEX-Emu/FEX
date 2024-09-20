// SPDX-License-Identifier: MIT
#include <FEXCore/Core/Thunks.h>
#include <FEXCore/fextl/memory.h>

namespace FEX::HLE {
struct ThreadStateObject;

class ThunkHandler : public FEXCore::ThunkHandler {
public:
  virtual void RegisterTLSState(FEXCore::Context::Context* CTX, FEX::HLE::ThreadStateObject* ThreadObject) = 0;
};
fextl::unique_ptr<ThunkHandler> CreateThunkHandler();
} // namespace FEX::HLE
