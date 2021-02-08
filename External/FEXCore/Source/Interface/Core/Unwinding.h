#include "Interface/Core/InternalThreadState.h"

/**
   * @brief Cancel current execution and exit via dispatcher
   *
   * @param CurrentThread Must be the thread state of this thread.
   *
   * Never returns.
   * All stack frames are cleaned up and execution is transferred
   * directly to the dispatcher, which then exits.
   */
[[noreturn]] void CancelToDispatcher(FEXCore::Core::InternalThreadState *CurrentThread);