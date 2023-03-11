# Deferred signals and why FEX needs them

FEX-Emu has locations in its code which are effectively "uninterruptible". In the sense that if the guest application receives a signal during an
"uninterruptible" code section, then FEX is likely to hang or crash in spurious and terrible ways.

## Example
FEX-Emu is in the middle of emitting code. This is a vulnerable state in which FEX can be in the middle of allocating memory or reading guest state.
If a signal is received in the middle of this, FEX-Emu might need to compile new code in a re-entrant state. Which could involve allocating memory and
other vulnerable things. In this case a mutex could very easily be held, interrupted, and then try to get held again while being reentrant.

**This will result in a hang.**

## How do we solve this?

### Classical signal masking
One solution to this problem is to mask **all** signals going in to an uninterruptible section and then unmask when leaving. This is the classical
approach that is viable if performance isn't a significant concern. A major problem with this approach is that we must do two system calls per
"uninterruptible" code section, which if the section is fairly small then it might cost more to ensure state integrity than doing the work at all.

### Cooperative signal deferring
A new solution is to defer asynchronous signals if they are caught inside an uninterruptible section. At the basic level, we increment a reference
counter going in to the "uninterruptible" section, and then decrement the reference counter once we leave. This way when the signal handler receives a
signal, it can check that thread's reference counter, store the `siginfo_t` to an array/stack object, and return to the same code segment to be handled
later.

The trick to how this works is the cost it takes to check if a signal occured, and then handling the signal safely. Ideally no signal has happened in
the "uninterruptible" code section, so the check needs to be as cheap as possible to ensure no cost in the case that no signal has occured.

The trick is that FEX maintains two memory regions for tracking deferred signals **per thread**.

#### 1st memory region

This region is FEX's InternalThreadState object, which is always resident for each guest thread and usually inside a register inside the JIT.
Inside this object is where the reference counter for "uninterruptible" code segments lives. It is specifically a reference counter since these
code segments may nest inside each other and we can only interrupt with a signal if the counter is zero.

This reference counter is thread local and won't be read by any other threads, so it can be a non-atomic increment and decrement.
Meaning it is usually three instructions (on ARM64) to increment and decrement.

```cpp
MoveableNonatomicRefCounter<uint64_t> DeferredReferenceCounter;
MoveableNonatomicRefCounter<uint64_t> *DeferredSignalHandlerPagePtr;
```

#### 2nd memory region

This memory region is a single page of memory that is allocated per thread. A pointer to this region exists inside the InternalThreadState object just
after the reference counter. This allows the JIT code to quickly load the pointer to this region after modifying the reference counter.

This is the tricky memory page where we determine if we have any deferred signals to process. I say it's tricky because after FEX leaves an
"uninterruptible" code section, it will decrement the reference counter, and then try to store in to the first byte of this page. In the case that no
signal has been deferred, this memory store will progress without issue, consuming only two instructions in the process.

In the case that a signal **HAS** been deferred, then the permissions on this page will be set to `PROT_NONE` and this memory store will cause a
SIGSEGV. Then FEX's signal handler will check to see if the access was this special page and start the deferred signal mechanisms. This means that a
deferred signal check is only five instructions in total.

#### Example ARM64 JIT code for uninterruptible region
```asm
  ; Increment the reference counter.
  ldr x0, [x28, #(offsetof(CPUState, DeferredReferenceCounter))]
  add x0, x0, #1
  str x0, [x28, #(offsetof(CPUState, DeferredReferenceCounter))]

  ; Do the uninterruptible code section here.
  <...>

  ; Now decrement the reference counter.
  ldr x0, [x28, #(offsetof(CPUState, DeferredReferenceCounter))]
  sub x0, x0, #1
  str x0, [x28, #(offsetof(CPUState, DeferredReferenceCounter))]

  ; Now the magic memory access to check for any deferred signals.
  ; Load the page pointer from the CPUState
  ldr x0, [x28, #(offsetof(CPUState, DeferredSignalHandlerPagePtr))]
  ; Just store zero. (1 cycle plus no dependencies on a register. Super fast!)
  ; Will store fine with no deferred signal, or SIGSEGV if there was one!
  str xzr, [x0]
```

### Deferred signal handling
In the case that FEX has received a signal, FEX's signal handler will first check to see if that thread's reference counter is zero or not.

#### Reference counter is zero
This is the easy case, just handle the signal as normal.

#### Reference counter is not zero
The signal handler now knows that FEX is in an uninterruptible code section. We check the signal to see if it is a synchronous signal or not.
- If the signal is synchronous then we need to handle it as normal, because this is a hardware signal that we can't defer.
- If it is an async signal (from tgkill, sigqueue, or something else) then we will start the deferring process.

The deferring process starts with storing the kernel `siginfo_t` to a thread local array so we can restore it later.
We then modify the permissions on the thread local `DeferredSignalHandlerPagePtr` to be `PROT_NONE`.
We then immediately return from the signal handler so that FEX can resume its "uninterruptible" code section without breaking anything.
Once the "uninterruptible" code section finishes, FEX will intentionally trigger a SIGSEGV by storing to the page.

This will trigger a jump to FEX's SIGSEGV handler, where FEX will process the signal as if it was the previously deferred signal.
the previous signal that was deferred.
- Replacing the SIGSEGV signal number with the previous captured signal number
- Replacing the `siginfo_t` with the previous captured `siginfo_t`
- mprotect the thread local `DeferredSignalHandlerPagePtr` to be RW.
   - This is so that future signals get deferred, but we don't block forward code progress.
- TODO: Overwrite mcontext_t things? I don't think this matters but there will be some private state that might leak SIGSEGV information?

Once we are now handling the guest signal, FEX-Emu is in a vulnerable state where any signals received will be deferred /and/ not handled at the end
of "uninterruptible" code sections. This is because FEX is now currently in a guest signal frame and we need to handle code compiling and other
potentially awkward interactions without checking for additional signals.

Once a guest signal handler has finished what it was working on, it will call `rt_sigreturn` or `sigreturn` which triggers FEX's SIGILL signal
handler.
   - This SIGILL behaviour is how FEX-Emu emulates sigreturn. In order to safely long-jump on AArch64, it must come from a signal context.
   - The sigreturn syscall handlers intentionally trigger a SIGILL to do this.

Inside of this SIGILL signal handler FEX will restore the state of FEX /back/ to where the deferred signal handler started (The str xzr, [x0]).
Inside of this signal handler FEX will check to see if all deferred signals are handled.
- Checks the reference counter to see if it is zero or not.
   - If further asynchronous signals have been triggered that need handling, mprotect the fault page to `PROT_NONE`
      - This trampolining is repeated once per asynchronous signal queued during processing.
      - This will cause further signal handling immediately once the JIT returns to its original location (Where it'll cause a SIGSEGV again).

Once FEX gets back to the page store, it will trampoline back to the SIGSEGV handler if it has more signals to handle.
   - This is an edge case where we aren't expecting multiple signals in almost all cases
   - Slightly more expensive is fine in this case.

## Disadvantages of cooperative signal deferring
Still thinking about this, come back to me. I'm concerned about signal queueing.
- How do we handle the guest doing a long-jump out of a signal frame and still receiving signals?
   - This will block FEX from handling /any/ more deferred signals.
   -  Do we need to store guest stack as well to see if it has reset its own stack frame?
   - moon-buggy does this as an example
   - We currently just leak stack for every guest signal handler that long jumps out of the signal frame.
      - Long term this would exhaust our stack and then crash.
      - Test with a second guest thread where our host will only have an 8MB stack instead of the 128MB primary stack.
      - See issue #2487
## Expectations and considerations
### What happens with a race condition with the refcounter?
There are two edges to this problem. The incrementing edge and the decrementing edge that must be considered.

#### Incrementing edge
This is the most problematic edge. This takes three instructions (one on x86) to increment the ref counter. If a signal is received between the load
and store then this theoretically could result in a tear on the refcounter. In actual practice this is a real tear but doesn't cause any problems.

The reasoning for this is that FEX isn't in the "uninterruptible" section until that reference counter has been stored, so FEX will handle the signals
immediately at that point, return to this code location, and then increment the counter. In particular, once returning to the code location the
refcounter will be the original value loaded. So even though it is a tear, it's one that doesn't cause issues since it is all thread local.

#### Decrementing edge
This edge is far less problematic to understand compared to the incrementing edge. Signals will get deferred entirely until the store instruction (If
storing zero), so FEX will always return to the code region and finish the decrement.

If FEX receives a signal after the decrement store has completed but /before/ the page faulting store has occured, then FEX will start processing the
signal immediately. At which point the fault page will have either RW or NONE permission. FEX will then likely hit another "uninterruptible" code
section which will complete the store to the fault page.
 - RW permission if it hadn't received another signal in the uninterruptible section
 - NONE permission if it did receive a signal previously

RW permission has no problems, it will continue as normal.

NONE will get captured by the fault handler, the fault handler will determine that there was no deferred signals, and set the fault page back to RW
permissions and continue execution safely.

## Execution examples
### No signal
This is a simple example because nothing happens.
``` diff
- <Enter Deferred region - 3 instructions>
! Compiling JIT Code
+ <Exit deferred region - 5 instructions>
```

### Signal outside of region
This is simple because the JIT just handles it.
``` diff
! In JIT code
# Signal received
# Guest Signal handler called
# JIT jumps to guest signal handler
# Hopefully guest calls rt_sigreturn instead of long jumping out.
```

### Synchronous signal in JIT
Deferred signals don't affect anything here because only asynchronous signals get affected.
* State reconstruction problems aren't discussed here.
``` diff
! In JIT code
! JIT code causes a synchronous signal (SIGSEGV or other)
# Guest Signal Handler called
# JIT jumps to guest signal handler
# Hopefully guest calls rt_sigreturn instead of long jumping out.
```

### Asynchronous signal in code emitter
This is the first interesting example since deferred signals affects it.
``` diff
- <Enter Deferred region - 3 instructions>
! Compiling JIT Code
# Asynchronous Signal received
# - Host signal handler determines the thread is in a deferred signal section.
# - Signal information is stored in a queue
# - mprotect signal page to NONE.
# - Signal handler returns without giving the signal to the guest
! Compiling JIT Code continues.
+ #1 - <Exit deferred region - 5 instructions>
+ Deferred region section causes SIGSEGV
+ - Host signal handler determines deferred region is done, Still has signal in queue.
+ - Pull signal information off of queue
# JIT jumps to guest signal handler
# Hopefully guest calls rt_sigreturn instead of long jumping out.
# Host PC is back at deferred signal section.
+ Deferred region section causes SIGSEGV #2
+ - Host signal handler determines deferred region is done, No signals in the queue.
# - mprotect signal page to RW.
# - Continue execution.
+ #1 <Exit deferred region continues>
```

### Recursive regions with signal in code emitter.
This one mostly matches the previous example except the behaviour of deferred signal regions leaving.

In this case, if the thread-local refcount is still >0 on `<Exit deferred region>` then there are two behaviours.
- On ARM64, it will receive a SIGSEGV but the signal handler will increment PC by one instruction and continue execution
   - Expectation is that signals are significantly less common than `<Exit deferred region>` so the cost of SIGSEGV+PC increment is faster.
- On x86-64, the region exit checks the refcount before doing the fault access.
   - This adds more instructions so is slower on average.

This has the expectation that recursive deferred regions both aren't very deep (usually only nested a couple times), and that signals are rare.
This way there aren't many SIGSEGV checks generated and the signal is finally only handled when reaching the top-most deferred region exit routine.

``` diff
- #1 <Enter Deferred region - 3 instructions>
! Compiling JIT Code
-    #2 <Enter Deferred region - 3 instructions>
!    Memory allocation
#    <Async signal received logic from above>
+    #2 <Exit deferred region - 5 instructions>
+    #2 Exit deferred region causes SIGSEGV
+    - TLS refcount is still 1 (from #1)
+    - PC is incremented by one instruction, signal still unhandled.
+ #1 <Exit deferred region - 5 instructions>
+ #1 Exit deferred region causes SIGSEGV
+ <Regular deferred region handling from above called>
```

### Multiple signals in deferred region
This is slightly different from the previous iterations since multiple signals in the stack result in odd behaviour.

``` diff
- #1 <Enter Deferred region - 3 instructions>
! Compiling JIT Code
# #1 Asynchronous Signal received
#   - Signal queued logic
# #2 Asynchronous Signal received
#   - Signal queued logic
+ #1 <Exit deferred region - 5 instructions>
+ Exit deferred region causes SIGSEGV
+ <Regular deferred region handling from above called>
+ Guest calls rt_sigreturn
+ - rt_sigreturn handler checks for number of queued signals
# - mprotect signal page to NONE because signals is > 0
# - JIT is back to #1 <Exit deferred region>
# - Exit deferred region causes SIGSEGV again.
# - Regular handler loop occurs
```

