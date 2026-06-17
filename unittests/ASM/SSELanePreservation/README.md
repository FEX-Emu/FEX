These unit-tests aim to ensure that SSE operations won't accidentally overwrite the upper 128-bit lane
on a system where SVE-256 is available. The underlying problem when handling SSE and AVX on an ARM system,
is that Adv. SIMD on ARM will always zero-extend registers that are participating in an arithmetic operation
(and others as well in some cases), whereas SSE on x86 does *not* perform this behavior. SSE leaves the upper
half (or upper three quadwords, if using AVX-512) untouched.

Therefore, to avoid the issue of the upper vector lanes being trounced, what we do is perform an insert
if an SSE operation happens to execute on a system that has SVE-256, preserving data in the upper lanes.

All of the tests in this folder aim to prevent changes where that guarantee is violated from slipping through.
