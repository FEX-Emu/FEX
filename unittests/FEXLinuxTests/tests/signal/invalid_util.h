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

// Signal handler that writes its context data to the global from_handler
static void CapturingHandler(int signal, siginfo_t *siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  from_handler = { _context->uc_mcontext, signal, siginfo->si_code };
#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] += capturing_handler_skip;
#undef FEX_IP_REG
}
