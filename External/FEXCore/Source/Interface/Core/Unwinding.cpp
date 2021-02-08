#include <unwind.h> // FIXME grab from somewhere else

#include "Interface/Core/Unwinding.h"
#include <FEXCore/Utils/LogManager.h>

// This is semi-generic to platforms that implement SysV ABIs with the Unwind Library interface
// see https://www.uclibc.org/docs/psABI-x86_64 page 85
//
// If you are porting to another platform, you will need to rewrite this

_Unwind_Reason_Code StopFn(int version, _Unwind_Action actions, _Unwind_Exception_Class exceptionClass,
 _Unwind_Exception *exceptionObject, struct _Unwind_Context *context, void *arg) {
     // This function is called by the unwinder at each stackframe.

    auto Thread = reinterpret_cast<FEXCore::Core::InternalThreadState*>(arg);
    auto ip = _Unwind_GetIP(context);

    // We check if the current stackframe belongs to either the dispatcher or jit code
    if (Thread->CPUBackend->IsAddressInJITCode(ip, true)) {
         // We have now unwound to the correct stack frame

         _Unwind_DeleteException(exceptionObject); // Cleanup

        // Longjump to dispatcher's exit
        (*Thread->LongJumpExit)(Thread);
        // Does not return
     }

    // Otherwise, continue unwinding
     return _URC_NO_REASON;
}

void CleanupFn(_Unwind_Reason_Code,  _Unwind_Exception *exception) {
    delete exception;
}

void CancelToDispatcher(FEXCore::Core::InternalThreadState *CurrentThread) {

    // We can't allocate this on the stack, because we are about to unwind the stack
    _Unwind_Exception *e = new _Unwind_Exception {'FEXs', CleanupFn, 0, 0};

    // Ask the ABI to forceably unwind the stack, calling our StopFn at each step
    _Unwind_ForcedUnwind(e, StopFn, CurrentThread);

    for (;;); // unreachable
}
