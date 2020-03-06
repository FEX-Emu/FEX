# FEXCore IR Optimization passes
---
**This is very much a WIP since these optimization passes aren't in code yet**
## Pass Managers
* Need Function level optimization pass manager
* Need block level optimization pass manager
### Dead Store Elimination
We need to do dead store elimination because LLVM can't always handle elimination of our loadstores
This is very apparent when we are doing flag calculations and LLVM isn't able to remove them
This is mainly just an issue around the context loadstores.
We will want this more when the IRJIT comes online.
### Dead flag elimination
X86-64 is a fairly disguting ISA in that it calculates a bunch of flags on almost all instructions.
We need eliminate redundant flag calculations that end up being being overwritten without being used.
This happens *constantly* and in most cases the flag calculation takes significantly more work than the basic op by itself
Good chance that breaking out the flags to independent memory locations will make this easier. Or just adding ops for flag handling.
### Dead Code Elimination
There are a lot of cases that code will be generated that is immediately dead afterwards.
Flag calculation elimination will produce a lot of dead code that needs to get removed.
Additionally there are a decent amount of x86-64 instructions that store their results in to multiple registers and then the next instruction overwrites one of those instructions.
Multiply and Divide being a big one, since x86 calculates these at higher precision.
These can rely significantly tracking liveness between LoadContext and StoreContext ops
### ABI register elimination pass
This one is very fun and will reduce a decent amount of work that the JIT needs to do.
When we are targeting a specific x86-64 ABI and we know that we have translated a block of code that is the entire function.
We can eliminate stores to the context that by ABI standards is a temporary register.
We will be able to know exactly that these are dead and just remove the store (and run all the passes that optimize the rest away afterwards).
### Loadstore coalescing pass
Large amount of x86-64 instructions load or store registers in order from the context.
We can merge these in to loadstore pair ops to improve perf
### Function level heuristic pass
Once we know that a function is a true full recompile we can do some additional optimizations.
Remove any final flag stores. We know that a compiler won't pass flags past a function call boundry(It doesn't exist in the ABI)
Remove any loadstores to the context mid function, only do a final store at the end of the function and do loads at the start. Which means ops just map registers directly throughout the entire function.
### SIMD coalescing pass?
When operating on older MMX ops(64bit SIMD) and they may end up up generating some independent ops that can be coalesced in to a 128bit op
