# Deferred signals and why FEX needs them

FEX-Emu has locations in its code which are effectively "uninterruptible". In the sense that if the guest application receives a signal during an
"uninterruptible" code section, then FEX is likely to hang or crash in spurious and terrible ways.

## Example
When FEX is in the process of emitting code, it often needs to acquire mutexes to safeguard operations like memory allocations or reading guest state.
This puts FEX in a vulnerable state: If a signal is received in the middle of this, FEX may need to initiate compilation of new code. In this case a
mutex could already be held, so attempting to acquire it again would trigger a deadlock.

## How do we solve this?

### Classical signal masking
One solution to this problem is to mask **all** signals going in to an uninterruptible section and then unmask when leaving. This is the classical
approach that is viable if performance isn't a significant concern. A major problem is that it requires two system calls per "uninterruptible" code
section, which adds overhead that may exceed the runtime of the section itself.

### Cooperative signal deferring
A new solution is to defer asynchronous signals caught inside an uninterruptible section and handle them at the end of that section.

At the basic level, we increment a reference counter going in to the "uninterruptible" section, and then decrement the reference counter once we leave.
This way when the signal handler receives a signal, it can check that thread's reference counter, store the `siginfo_t` to an array/stack object, and
return to the same code segment to be handled later.

By making this check as cheap as possible, overhead is minimized for the general case that no signal occurs during "uninterruptible" sections. FEX
achieves this by maintaining two memory regions for tracking deferred signals **per thread**.

#### 1st memory region

This region is FEX's InternalThreadState object, which is always resident for each guest thread and usually inside a register inside the JIT.
Inside this object is where the reference counter for "uninterruptible" code segments lives. It is specifically a reference counter since these
code segments may nest inside each other and we can only interrupt with a signal if the counter is zero.

This reference counter is thread local and won't be read by any other threads, so it can be a non-atomic increment and decrement.
Meaning it is usually three instructions (on ARM64) to increment and decrement.

```cpp
NonAtomicRefCounter<uint64_t> DeferredSignalRefCount;
```

#### 2nd memory region

This memory region is a single page of memory that is allocated per thread. Its purpose is to trigger a SIGSEGV when FEX leaves an "uninterruptible"
section if a signal has been deferred. FEX's signal handler will check if the faulting address is in this special page and subsequently starts the
deferred signal mechanisms.

```cpp
NonAtomicRefCounter<uint64_t> *DeferredSignalFaultAddress;
```

#### Example ARM64 JIT code for uninterruptible region
```asm
  ; Increment the reference counter.
  ldr x0, [x28, #(offsetof(CPUState, DeferredSignalRefCount))]
  add x0, x0, #1
  str x0, [x28, #(offsetof(CPUState, DeferredSignalRefCount))]

  ; Do the uninterruptible code section here.
  <...>

  ; Now decrement the reference counter.
  ldr x0, [x28, #(offsetof(CPUState, DeferredSignalRefCount))]
  sub x0, x0, #1
  str x0, [x28, #(offsetof(CPUState, DeferredSignalRefCount))]

  ; Now the magic memory access to check for any deferred signals.
  ; Load the page pointer from the CPUState
  ldr x0, [x28, #(offsetof(CPUState, DeferredSignalFaultAddress))]
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
We then modify the permissions on the thread local `DeferredSignalFaultAddress` to be `PROT_NONE`.
We then immediately return from the signal handler so that FEX can resume its "uninterruptible" code section without breaking anything.
Once the "uninterruptible" code section finishes, FEX will intentionally trigger a SIGSEGV by storing to the page.

Once FEX-Emu is in its SIGSEGV handler, it will determine that it is handling a deferred signal. This will pull the previously saved `siginfo_t` and
start processing the signal.

Once a guest signal handler has finished what it was working on, it will call `rt_sigreturn` or `sigreturn` which triggers FEX's SIGILL signal
handler.

Inside of this SIGILL signal handler FEX will restore the state of FEX /back/ to where the deferred signal handler started (The str xzr, [x0]).
Then, FEX will check if any further deferred signals need to be handled.
- Checks if the reference counter is zero or not
   - If further asynchronous signals have been triggered that need handling, mprotect the fault page to `PROT_NONE`
      - This trampolining is repeated once per asynchronous signal queued during processing.
      - This will cause further signal handling immediately once the JIT returns to its original location (where it'll cause a SIGSEGV again).

Once FEX gets back to the page store, it will trampoline back to the SIGSEGV handler if it has more signals to handle.

## Disadvantages of cooperative signal deferring
- How do we handle the guest doing a longjmp out of a signal frame and still receiving signals?
   - FEX relies on guest signal handlers returning via `sigreturn` to handle stacked deferred signals, so a longjmp would interfere with this
   -  Do we need to store guest stack as well to see if it has reset its own stack frame?
   - moon-buggy does this as an example
   - We currently just leak stack for every guest signal handler that long jumps out of the signal frame.
      - Long term this would exhaust our stack and then crash.
      - Test with a second guest thread where our host will only have an 8MB stack instead of the 128MB primary stack.
      - See issue #2487
- Deeply recursive signal deferring sections can have excessive SIGSEGV faults.
   - In the case of ARM64 it will do a SIGSEGV at the end of each deferred signal section if a signal is queued.
   - This can result in a bunch of trampolining.
   - Just make sure to not do excessive nesting of deferred signal sections.
   - Typically not a problem since deferred signals aren't common.

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

If FEX receives a signal after the decrement store has completed but /before/ the page faulting store has occurred, then FEX will start processing the
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

- **Enter Deferred region - 3 instructions**
- Compiling JIT Code
- **Exit deferred region - 5 instructions**

### Signal outside of region
This is simple because the JIT just handles it.

- In JIT code
- Signal received
- Guest Signal handler called
- JIT jumps to guest signal handler
- Hopefully guest calls rt_sigreturn instead of long jumping out.

### Synchronous signal in JIT
Deferred signals don't affect anything here because only asynchronous signals get affected.

- In JIT code
- JIT code causes a synchronous signal (SIGSEGV or other)
- Guest Signal Handler called
- JIT jumps to guest signal handler
- Hopefully guest calls rt_sigreturn instead of long jumping out.

### Asynchronous signal in code emitter
This is the first interesting example since deferred signals affects it.

- **Enter Deferred region - 3 instructions**
- Compiling JIT Code
- Asynchronous Signal received
  - Host signal handler determines the thread is in a deferred signal section.
  - Signal information is stored in a queue
  - mprotect signal page to NONE.
  - Signal handler returns without giving the signal to the guest
- Compiling JIT Code continues.
- **Exit deferred region - 5 instructions**
- Deferred region section causes SIGSEGV
  - Host signal handler determines deferred region is done, Still has signal in queue.
  - Pull signal information off of queue
- JIT jumps to guest signal handler
- Hopefully guest calls rt_sigreturn instead of long jumping out.
- Host PC is back at deferred signal section.
- Deferred region section causes SIGSEGV #2
  - Host signal handler determines deferred region is done, No signals in the queue.
  - mprotect signal page to RW.
  - Continue execution.
- **Exit deferred region continues**

### Recursive regions with signal in code emitter.
This one mostly matches the previous example except the behaviour of deferred signal regions leaving.

In this case, if the thread-local refcount is still >0 on `<Exit deferred region>` then there are two behaviours.
- On ARM64, it will receive a SIGSEGV but the signal handler will increment PC by one instruction and continue execution
   - Expectation is that signals are significantly less common than `<Exit deferred region>` so the cost of SIGSEGV+PC increment is faster.
- On x86-64, the region exit checks the refcount before doing the fault access.
   - This adds more instructions so is slower on average.

This has the expectation that recursive deferred regions both aren't very deep (usually only nested a couple times), and that signals are rare.
This way there aren't many SIGSEGV checks generated and the signal is finally only handled when reaching the top-most deferred region exit routine.

- **Enter Deferred region - 3 instructions**
- Compiling JIT Code
     - Enter Deferred region - 3 instructions
     - Memory allocation
     - Async signal received logic from above
     - Exit deferred region - 5 instructions
     - Exit deferred region causes SIGSEGV
     - TLS refcount is still 1
     - PC is incremented by one instruction, signal still unhandled.
- **Exit deferred region - 5 instructions**
- Exit deferred region causes SIGSEGV
- **Regular deferred region handling from above called**

### Multiple signals in signal-deferring region
This is slightly different from the previous iterations since multiple signals in the stack result in odd behaviour.

- **Enter Deferred region - 3 instructions**
- Compiling JIT Code
- Asynchronous Signal received
    - Signal queued logic
- Asynchronous Signal received
    - Signal queued logic
- **Exit deferred region - 5 instructions**
- Exit deferred region causes SIGSEGV
- **Regular deferred region handling from above called**
- Guest calls rt_sigreturn
  - rt_sigreturn handler checks for number of queued signals
  - mprotect signal page to NONE because signals is > 0
  - JIT is back to **Exit deferred region**
  - Exit deferred region causes SIGSEGV again.
  - Regular handler loop occurs
