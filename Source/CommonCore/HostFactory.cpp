#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>

#ifdef _M_X86_64
#include <asm/ldt.h>
#include <sys/syscall.h>
#endif
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <ucontext.h>
#include <vector>

#include <signal.h>
#include <xbyak/xbyak.h>
using namespace Xbyak;

namespace HostFactory {
#ifdef _M_X86_64
  static inline int modify_ldt(int func, void *ldt) {
    return ::syscall(SYS_modify_ldt, func, ldt, sizeof(struct user_desc));
  }

  class HostCore final : public FEXCore::CPU::CPUBackend, public Xbyak::CodeGenerator {
  public:
    explicit HostCore(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread, bool Fallback);
    ~HostCore() override;
    std::string GetName() override { return "Host Core"; }
    void* CompileCode(uint64_t Entry, FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) override;

    void *MapRegion(void *HostPtr, uint64_t VirtualGuestPtr, uint64_t Size) override {
      return HostPtr;
    }

    void Initialize() override;
    bool NeedsOpDispatch() override { return false; }

    bool HandleSIGSEGV(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext);

  private:
    FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
    int CodeSegmentEntry{};
    int GlobalCodeSegmentEntry{};
    uint64_t GetCodeSegmentEntryLocation;

    uint64_t ReturningStackLocation;
    uint64_t ThreadStopHandlerAddress;
    void Setup32BitCodeSegment();
    uint32_t MakeSelector(int Segment, bool LDT) const {
      // Selector Index, Table Indicator (1 = LDT, 0 = GDT), CPL (3 = userland)
      return (Segment << 3) | ((uint32_t)LDT << 2) | 3;
    };
  };

  HostCore::~HostCore() {
  }

  HostCore::HostCore(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread, bool Fallback)
    : CPUBackend(Thread, 0, 0)
    , CodeGenerator(4096) {
    FEXCore::Context::RegisterHostSignalHandler(CTX, SIGSEGV,
      [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
        auto InternalThread = Thread;
        HostCore *Core = reinterpret_cast<HostCore*>(InternalThread->CPUBackend.get());
        return Core->HandleSIGSEGV(Thread, Signal, info, ucontext);
      },
      true
    );

    FEXCore::Context::RegisterHostSignalHandler(CTX, 63, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      return true;
    }, true);

    Setup32BitCodeSegment();
  }

  bool HostCore::HandleSIGSEGV(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Check our current instruction that we just executed to ensure it was an HLT
    uint8_t *Inst{};


    Inst = reinterpret_cast<uint8_t*>(_mcontext->gregs[REG_RIP]);
    if (!Is64BitMode()) {
      if (_mcontext->gregs[REG_RIP] == GetCodeSegmentEntryLocation) {
        // Backup the CSGSFS register
        GlobalCodeSegmentEntry = _mcontext->gregs[REG_CSGSFS];
        // Skip past this hlt and keep running
        _mcontext->gregs[REG_RIP] += 1;
        return true;
      }
    }
    constexpr uint8_t HLT = 0xF4;
    if (Inst[0] != HLT) {
      return false;
    }

    // Store our host state in to the guest for testing against
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RAX] = _mcontext->gregs[REG_RAX];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RBX] = _mcontext->gregs[REG_RBX];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RCX] = _mcontext->gregs[REG_RCX];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RDX] = _mcontext->gregs[REG_RDX];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RBP] = _mcontext->gregs[REG_RBP];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSI] = _mcontext->gregs[REG_RSI];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RDI] = _mcontext->gregs[REG_RDI];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP] = _mcontext->gregs[REG_RSP];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R8]  = _mcontext->gregs[REG_R8];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R9]  = _mcontext->gregs[REG_R9];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R10] = _mcontext->gregs[REG_R10];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R11] = _mcontext->gregs[REG_R11];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R12] = _mcontext->gregs[REG_R12];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R13] = _mcontext->gregs[REG_R13];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R14] = _mcontext->gregs[REG_R14];
    Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_R15] = _mcontext->gregs[REG_R15];
    Thread->CurrentFrame->State.rip                               = _mcontext->gregs[REG_RIP];

    for (size_t i = 0; i < 16; ++i) {
      memcpy(&Thread->CurrentFrame->State.xmm[i], &_mcontext->fpregs->_xmm[i], sizeof(_mcontext->fpregs->_xmm[0]));
    }

    uint16_t CurrentOffset = (_mcontext->fpregs->swd >> 11) & 7;

    for (size_t i = 0; i < 8; ++i) {
      memcpy(&Thread->CurrentFrame->State.mm[(i + CurrentOffset) % 8], &_mcontext->fpregs->_st[i], sizeof(_mcontext->fpregs->_st[0]));
    }

    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the JIT and get out safely
    _mcontext->gregs[REG_RSP] = ReturningStackLocation;

    // Set the new PC
    _mcontext->gregs[REG_RIP] = ThreadStopHandlerAddress;

    if (!Is64BitMode()) {
      // Unset code segment so we can jump back in to 64-bit mode
      _mcontext->gregs[REG_CSGSFS] = GlobalCodeSegmentEntry;
    }

    return true;
  }

  void HostCore::Setup32BitCodeSegment() {
    if (Is64BitMode()) {
      return;
    }

    struct user_desc ldt{};
    ldt.entry_number = 1;
    // This is where HarnessCodeLoader loads code to
    ldt.base_addr = 0;
    ldt.limit = ~0U; // No limit
    ldt.seg_32bit = 1; // 32-bit
    ldt.contents = MODIFY_LDT_CONTENTS_CODE;
    ldt.read_exec_only = 0;
    ldt.limit_in_pages = 1;
    ldt.seg_not_present = 0;
    ldt.useable = 1;
    ldt.lm = 0; // Not-64-bit
    int Res = modify_ldt(0x11, &ldt);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't load 32-bit LDT");
      return;
    }

    CodeSegmentEntry = MakeSelector(ldt.entry_number, 1);

    // Make the data segment follow directly after the code segment
    // Overlapping region makes it read/write
    ldt.entry_number = 2;
    // This is where HarnessCodeLoader loads code to
    ldt.base_addr = 0;
    ldt.limit = ~0U; // No limit
    ldt.seg_32bit = 1; // 32-bit
    ldt.contents = MODIFY_LDT_CONTENTS_DATA;
    ldt.read_exec_only = 0;
    ldt.limit_in_pages = 1;
    ldt.seg_not_present = 0;
    ldt.useable = 1;
    ldt.lm = 0; // Not-64-bit
    Res = modify_ldt(0x11, &ldt);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't load 32-bit LDT");
      return;
    }

    // Stack entry overlapping data
    ldt.entry_number = 3;
    // This is where HarnessCodeLoader loads code to
    ldt.base_addr = 0;
    ldt.limit = ~0U; // No limit
    ldt.seg_32bit = 1; // 32-bit
    ldt.contents = MODIFY_LDT_CONTENTS_STACK;
    ldt.read_exec_only = 0;
    ldt.limit_in_pages = 1;
    ldt.seg_not_present = 0;
    ldt.useable = 1;
    ldt.lm = 0; // Not-64-bit
    Res = modify_ldt(0x11, &ldt);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't load 32-bit LDT");
      return;
    }

  }

  void HostCore::Initialize() {
    DispatchPtr = getCurr<CPUBackend::AsmDispatch>();
    // x86-64 ABI has the stack aligned when /call/ happens
    // Which means the destination has a misaligned stack at that point
    push(rbx);
    push(rbp);
    push(r12);
    push(r13);
    push(r14);
    push(r15);
    sub(rsp, 8);

    // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
    // regardless of where we were in the stack
    mov(rax, (uint64_t)&ReturningStackLocation);
    mov(qword [rax], rsp);

    mov (rax, 0);
    mov (rbx, 0);
    mov (rcx, 0);
    mov (rdx, 0);
    mov (rbp, 0);
    mov (rsi, 0);
    mov (r8,  0);
    mov (r9,  0);
    mov (r10, 0);
    mov (r11, 0);
    mov (r12, 0);
    mov (r13, 0);
    mov (r14, 0);
    mov (r15, 0);
    finit();


    if (Is64BitMode()) {
      push(qword [rdi + offsetof(FEXCore::Core::CPUState, rip)]);

      // Load our RIP
      // RSP won't be set to zero here but should be fine
      ret();
    }
    else {
      // Far call needs to go through a gate
      // This is setup just like the following packing
      // {
      //  uint32_t RIP;
      //  uint16_t CodeSegment;
      // }

      GetCodeSegmentEntryLocation = getCurr<uint64_t>();
      hlt();

      Label Gate{};
      jmpf(ptr [rip + Gate]);

      L(Gate);
      dd(0x1'0000); // This is a 32-bit offset from the start of the gate. We start at 0x1'0000 + 0
      dw(CodeSegmentEntry);
    }

    ThreadStopHandlerAddress = getCurr<uint64_t>();

    add(rsp, 8);

    pop(r15);
    pop(r14);
    pop(r13);
    pop(r12);
    pop(rbp);
    pop(rbx);

    ret();
    ready();
  }

  void* HostCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
    return nullptr;
  }

  std::unique_ptr<FEXCore::CPU::CPUBackend> CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread) {
    return std::make_unique<HostCore>(CTX, Thread, false);
  }
#else
  std::unique_ptr<FEXCore::CPU::CPUBackend> CPUCreationFactory(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread) {
    LOGMAN_MSG_A_FMT("HostCPU factory doesn't exist for this host");
    return nullptr;
  }
#endif
}


