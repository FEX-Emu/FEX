// SPDX-License-Identifier: MIT
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <unistd.h>
#include <signal.h>

namespace FEXCore {
  void SignalDelegator::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
    SetHostSignalHandler(Signal, Func, Required);
    FrontendRegisterHostSignalHandler(Signal, Func, Required);
  }

  void SignalDelegator::HandleSignal(int Signal, void *Info, void *UContext) {
    // Let the host take first stab at handling the signal
    auto Thread = GetTLSThread();
    HostSignalHandler &Handler = HostHandlers[Signal];

    if (!Thread) {
      LogMan::Msg::AFmt("[{}] Thread has received a signal and hasn't registered itself with the delegate! Programming error!", FHU::Syscalls::gettid());
    }
    else {
      for (auto &Handler : Handler.Handlers) {
        if (Handler(Thread, Signal, Info, UContext)) {
          // If the host handler handled the fault then we can continue now
          return;
        }
      }

      if (Handler.FrontendHandler &&
          Handler.FrontendHandler(Thread, Signal, Info, UContext)) {
        return;
      }

      // Now let the frontend handle the signal
      // It's clearly a guest signal and this ends up being an OS specific issue
      HandleGuestSignal(Thread, Signal, Info, UContext);
    }
  }
}
