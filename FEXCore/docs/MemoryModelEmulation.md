# What is x86-TSO and what is different compared to ARM's weak memory model?
x86's memory model is a very strictly coherent memory model that effectively mandates that all memory accesses are "atomic". While atomicity is
actually a bit more strict, we actually need to emulate it in ARM using atomic instructions. We are also required to emulate this strictness with
unaligned accesses, which is due to x86 CPUs allowing unaligned atomics for "free" within a cacheline. Intel also takes this a step more and allowing
full atomics with a feature called "split-locks", AMD gains this same feature in Zen 5.

# Emulating loads
Due to x86 SIB addressing, this can happen on most instructions. FEX emulates these in a variety of ways depending on features.
Most instructions are emulated with an atomic instruction but we also implement a feature called "half-barrier" atomics for unaligned atomics.

## Base ARMv8.0
- Addressing limitations
  - Register only
This is emulated using an atomic load instruction plus a nop.
- On unaligned access the code gets backpatched to a non-atomic load plus a memory barrier

## FEAT_LRCPC
- Addressing limitations
  - Register only
This matches the base ARMv8.0 implementation but adds new instructions that match x86-TSO behaviour, making the emulation slightly quicker.
- On unaligned access it still gets backpatched to non-atomic load plus a memory barrier.

## FEAT_LRCPC2
- Addressing limitations
  - Register plus 9-bit signed immediate (-256, 255)
Adds some new instructions that allow immediate encoding inside of the previous LRCPC instructions

## FEAT_LRCPC3
Adds a handful of GPR instructions that aren't super interesting

FEX doesn't currently implement these since no hardware supports it.

- ldapr - Post-index load for stack
- ldiapr - Post-index load pair for stack
- stilp - pre-index store pair for stack
- stlr - pre-index store for stack

# Emulating stores
Again due to x86 SIB addressing, this can also happen on most instructions. There are less options for FEX with this extension, so in most cases this
just turns in to an atomic store with half-barrier backpatching for unaligned accesses

## FEAT_LRCPC, FEAT_LRCPC2
Adds nothing for emulating stores

## FEAT_LRCPC3

# Emulating atomic instructions
x86 has atomic memory operations that can do a variety of operations. For unaligned atomic operations FEX will emulate the operation inside the signal
handler if it happens to be unaligned.

## CASPair - cmpxchg

## Base ARMv8.0
- Addressing limitations
  - Register only
This is emulated with a ldaxp+stlxp pair of instructions.

## FEAT_LSE
- Addressing limitations
  - Register only
Adds a new caspal instruction that does the operation almost exactly like x86.

## CAS - cmpxchg8b/cmpxchg16b

## Base ARMv8.0
- Addressing limitations
  - Register only
Similar to CASPair but now only uses a ldaxr+stlxr pair

## FEAT_LSE
- Addressing limitations
  - Register only
Similar to CASPair adds a new casal instruction that operates basically like x86

# AtomicFetch<Op>
## Op from the following list
- Add
- Sub
- And
- CLR
- Or
- Xor
- Neg
- Swap

## Base ARMv8.0
- Addressing limitations
  - Register only
All operations get emulated with an ldaxr+stlxr+<op> instruction

## FEAT_LSE
- Addressing limitations
  - Register only
Almost all operations now have a native atomic memory operation instruction. The only outlier is atomicNeg which doesn't have an LSE equivalent and
uses the ARMv8.0 implementation.

# Vector loads
Since almost all memory accesses on x86 are TSO, this includes vector operations.

## Base ARMv8.0
- Addressing limitations
  - Register plus 9-bit signed immediate (-256, 255)
  - Register plus 12-bit unsigned scaled immediate (Scaled by access size)
Emulated using half-barriers, which means a load+dmb

## FEAT_LRCPC3
- LDAP1 added for element loads. Register only address encoding
- LDAPUR added for vector register loads, supports 9-bit simm offset

# Vector stores
Just like loads, these are emulated using half-barriers

## Base ARMv8.0
- Addressing limitations
  - Register plus 9-bit signed immediate (-256, 255)
  - Register plus 12-bit unsigned scaled immediate (Scaled by access size)
Emulated using half-barriers, which means a dmb+str


## FEAT_LRCPC3
- STL1 added for element stores. Register only address encoding
- STLUR added for vector register stores, supports 9-bit simm offset

# Addressing limitations depending on operating mode
## GPR loadstores
### TSO Emulation disabled
- Register only (ldr/str)
- Register + Register + scale (ldr/str)
- Register + 9-bit simm (ldur/stru)
- Register + 12-bit unsigned scaled imm (ldr/str)

### TSO Emulation enabled
- Register only (ldar/stlr)
- Register only (ldapr/stlr) - FEAT_LRCPC
- Register + 9-bit simm (ldapr/stlur) - FEAT_LRCPC2

## Vector loadstores
### TSO Emulation disabled
- Register only (ldr/str)
- Register + Register + scale (ldr/str)
- Register + 9-bit simm (ldur/stru)
- Register + 12-bit unsigned scaled imm (ldr/str)

### TSO Emulation enabled
- Same as TSO emulation disabled due to half-barrier implementation

### TSO Emulation enabled (FEAT_LRCPC3)
- Register only (ldap1/stl1) - Element loadstore
- Register + 9-bit simm (ldapur/stlur)

## Atomic memory operations
Always TSO emulation enabled, always register only.
