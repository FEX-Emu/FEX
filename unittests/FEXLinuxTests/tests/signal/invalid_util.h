#include <cstdint>
#include <signal.h>
#include <optional>

struct CapturedHandlerState {
  mcontext_t mctx;
  int signal;
  int si_code;
};

std::optional<CapturedHandlerState> from_handler;

// Number of bytes to skip to resume from the signal handler
int capturing_handler_skip = 0;

// Number of times the signal handler has caught a signal
int capturing_handler_calls = 0;

// Signal handler that writes its context data to the global from_handler
static void CapturingHandler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  from_handler = {_context->uc_mcontext, signal, siginfo->si_code};
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] += capturing_handler_skip;
#undef FEX_IP_REG
  capturing_handler_calls++;
}

#if __SIZEOF_POINTER__ == 4
struct sigcontext_32 {
  uint16_t gs, gsh;
  uint16_t fs, fsh;
  uint16_t es, esh;
  uint16_t ds, dsh;
  uint32_t di;
  uint32_t si;
  uint32_t bp;
  uint32_t sp;
  uint32_t bx;
  uint32_t dx;
  uint32_t cx;
  uint32_t ax;
  uint32_t trapno;
  uint32_t err;
  uint32_t ip;
  uint16_t cs, csh;
  uint32_t flags;
  uint32_t sp_at_signal;
  uint16_t ss, ssh;

  uint32_t fpstate;
  uint32_t oldmask;
  uint32_t cr2;
};
struct sigframe_ia32 {
  uint32_t pretcode;
  int signal;
  sigcontext_32 sc;
  // <...>
  // Some extra state
};

struct rt_sigframe_ia32 {
  uint32_t pretcode;
  int signal;
  uint32_t pinfo;
  uint32_t puc;
  siginfo_t info;
  ucontext_t uc;
  // <...>
  // Some extra state
};

struct CapturedHandlerState_32 {
  sigcontext_32 mctx;
  int signal;
  int si_code;
};

struct CapturedHandlerState_regparm_32 {
  int signal;
  siginfo_t* siginfo;
  void* context;
};

// This capturing handler is for non-realtime signals pulling arguments from the stack.
std::optional<CapturedHandlerState_32> from_handler_32;
// This capturing handler is for non-realtime signals pulling arguments from regparm ABI.
std::optional<CapturedHandlerState_regparm_32> from_handler_regparm_32;

/*
 * This signal handler is for testing 32-bit non-realtime signal support.
 * This handler gives a signal, and a sigcontext_32 object.
 *
 * The arguments are passed on the stack for this function.
 */
static void CapturingHandler_non_realtime(int signal, ...) {
  // Getting the context frame is really hard, so hardwire some magic.
  // Getting the frame address returns
  // struct frame {
  //  uint32_t ????;
  //  uint32_t pret;
  //  uint32_t signal;
  //  sigcontext_32 sc;
  sigframe_ia32* frame = (sigframe_ia32*)((size_t)__builtin_frame_address(0) + 4);
  sigcontext_32* context = &frame->sc;

  from_handler_32 = {*context, signal, 0};
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  context->ip += capturing_handler_skip;
#undef FEX_IP_REG
  capturing_handler_calls++;
}

/*
 * This signal handler is for testing 32-bit realtime signal support.
 * This handler gives a signal, a siginfo_t, and mcontext_t object.
 *
 * The arguments are passed on the stack for this function.
 */
[[gnu::regparm(3)]]
static void CapturingHandler_realtime_regparm(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

  from_handler = {_context->uc_mcontext, signal, siginfo->si_code};
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] += capturing_handler_skip;
#undef FEX_IP_REG
  capturing_handler_calls++;
}

/*
 * This signal handler is for testing 32-bit non-realtime signal support.
 * This handler gives a signal, and that's it
 *
 * siginfo and context objects should always be nullptr in this case.
 *
 * The arguments are passed on in registers for this function.
 */
[[gnu::regparm(3)]]
static void CapturingHandler_non_realtime_regparm(int signal, siginfo_t* siginfo, void* context) {
  // Getting the context frame is really hard, so hardwire some magic.
  // Getting the frame address returns
  // struct frame {
  //  uint32_t ????;
  //  uint32_t pret;
  //  uint32_t signal;
  //  sigcontext_32 sc;
  // If volatile isn't used then the compiler optimizes this out.
  volatile sigframe_ia32* frame = (volatile sigframe_ia32*)((size_t)__builtin_frame_address(0) + 4);
  volatile sigcontext_32* context_stack = &frame->sc;

  // siginfo and context should be nullptr.
  from_handler_regparm_32 = {signal, siginfo, context};
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  context_stack->ip += capturing_handler_skip;
#undef FEX_IP_REG
  capturing_handler_calls++;
}

/*
 * This signal handler is for testing 32-bit realtime signal support.
 * This handler gives a signal, a siginfo_t, and mcontext_t object.
 *
 * The arguments are passed on the stack for this function.
 */
static void CapturingHandler_realtime() {
  // Getting the context frame is really hard, so hardwire some magic.
  // Getting the frame address returns
  // struct frame {
  //  uint32_t ????;
  //  rt_sigframe_ia32 frame;
  rt_sigframe_ia32* frame = (rt_sigframe_ia32*)((size_t)__builtin_frame_address(0) + 4);
  int signal = frame->signal;
  siginfo_t* siginfo = &frame->info;
  ucontext_t* _context = &frame->uc;

  from_handler = {_context->uc_mcontext, signal, siginfo->si_code};
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] += capturing_handler_skip;
#undef FEX_IP_REG
  capturing_handler_calls++;
}

/*
 * This signal handler is for testing 32-bit realtime signal support.
 * This handler gives a signal, a siginfo_t, and mcontext_t object.
 *
 * The arguments are passed on in registers for this function.
 * This one is specifically is for testing if the glibc handler is working correctly.
 * It matches `CapturingHandler_realtime_regparm` but without the `regparm` ABI.
 */
static void CapturingHandler_realtime_glibc_helper(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

  from_handler = {_context->uc_mcontext, signal, siginfo->si_code};
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] += capturing_handler_skip;
#undef FEX_IP_REG
  capturing_handler_calls++;
}
#endif
