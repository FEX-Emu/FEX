# FEXCore IR
---
The IR for the FEXCore is an SSA based IR that is generated from the incoming x86-64 assembly.
SSA is quite nice to work with when translating the x86-64 code to the IR, when optimizing that code with custom optimization passes, and also passing that IR to our CPU backends.

## Emulation IR considerations
* We have explicitly sized IR variables
  * Supports traditional element sizes of 1,2,4,8 bytes and some 16byte ops
  * Supports arbitrary number of vector elements
  * The op determines if something is float or integer based.
* Clear separation of scalar IR ops and vector IR ops
  * ex, MUL versus VMUL
* We have explicit Load/Store context IR ops
  * This allows us to have a clear separation between guest memory and tracked x86-64 state
* We have an explicit CPUID IR op
  * This allows us to return fairly complex data (4 registers of data) and also having an easier optimization for constant CPUID functions
  * So if we const-prop the CPUID function then it'll just const-prop further along
* We have an explicit syscall op
  * The syscall op is fairly complex as well, same with CPUID that if the syscall function is const-prop then we can directly call the syscall handler
  * Can save overhead by removing call overheads
* The IR supports branching from one block to another
  * Has a conditional branch instruction that either branches to the target branch or falls through to the next block
  * Has an unconditional branch to explicitly jump to a block instead of falling through
  * **There is a desire to follow LLVM semantics around block limitations but it isn't currently strictly enforced**
* Supports a debug `Print` Op for printing out values for debug viewing
* Supports explicit Load/Store memory IR ops
  * This is for accessing guest memory and will do the memory offset translation in to the VM's memory space
  * This is done by just adding the VM memory base to the 64bit address passed in
  * This is done in a manner that the application **can** escape from the VM and isn't meant to be safe
  * There is an option for JITs to validate the memory region prior to accessing for ensuring correctness
* IR is generated from a JSON file, fairly straightforward to extend.
  * Read the python generation file to determine the extent of what it can do

## IR function considerations
The first SSA node is a special case node that is considered invalid. This means %ssa0 will always be invalid for "null" node checks
The first real SSA node also has to be a IRHeader node. This means it is safe to assume that %ssa1 will always be an IRHeader.


```(%%ssa1) IRHeader 0x41a9a0, %%ssa2, 5```

The header provides information about that function like the entry point address.
Additionally it also points to the first `CodeBlock` IROp


```(%%ssa2) CodeBlock %%ssa7, %%ssa168, %%ssa3```


* The `CodeBlock` Op is a jump target and must be treated as if it'll be jumped to from other blocks
  * It contains pointers to the starting op and ending op and they are inclusive
  * It also contains a pointer to the next CodeBlock in a singly linked list
  * The last CodeBlock will point to the InvalidNode as the next block


### Example code block

```
(%%ssa3) CodeBlock %%ssa169, %%ssa173, %%ssa4
	(%%ssa169) BeginBlock %ssa3
	%ssa170 i64 = Constant 0x41a9e1
	(%%ssa171) StoreContext %ssa170 i64, 0x8, 0x0
	(%%ssa172) ExitFunction
	(%%ssa173) EndBlock %ssa3
```

* BeginBlock points back to the CodeBlock SSA which helps with iterating across multiple blocks
* EndBlock the ending op of a CodeBlock and also points back to the CodeBlock SSA.
* ExitFunction will leave the function immediately and return back to the dispatcher
* Every IR Op has an SSA value associated with it used for tracking the op itself
	* If the IROp doesn't have a real destination then it is invalid to use it as an argument in most other ops

## In-memory representation

The in-memory representation of the IR may be a bit confusing when initially viewed and once dealing with optimizations then it may be confusing as well.
Currently the IR Generation is tied to the `OpDispatchBuilder` class. This class handles translating decoded x86 to our IR representation.
When generating IR inside of the `OpDispatchBuilder` it is straight forward, just call the IR generation ops.

### FEXCore::IR::IntrusiveAllocator
This is an intrusive allocator that is used by the `OpDispatchBuilder` for storing IR data. It is a simple linear arena allocator without resizing capabilities.

### OpDispatchBuilder
OpDispatchBuilder provides two routines for handling the IR outside of the class
* `IRListView ViewIR();`
	* Returns a wrapper container class the allows you to view the IR. This doesn't take ownership of the IR data.
	* If the OpDispatcherBuilder changes its IR then changes are also visible to this class
* `IRListView *CreateIRCopy()`
	* As the name says, it creates a new copy of the IR that is in the OpDispatchBuilder
	* Copying the IR only copies the memory used and doesn't have any free space for optimizations after this copy operation
	* Useful for tiered recompilers, AOT, and offline analysis

This class uses two IntrusiveAllocator objects for tracking IR data. `ListData` and `Data` are the object names.
* `ListData` is for tracking the doubly linked list of nodes
	* This ONLY allocates `FEXCore::IR::OrderedNode` objects
	* When an OrderedNode is allocated its allocation location (NodeOffset) is just the offset from the base pointer
	* This allows us to only use uint32_t memory offsets to compact the IR
	* Additionally using offsets allows us the freedom to freely move our IR in memory without costly pointer adjustment
	* This means everything is fixed size allocated (SSA Node number calculation is just `AllocationOffset / sizeof(OrderedNode)`
	* OrderedNodes are what the SSA arguments are pointing to in the end


### OrderedNode
This is a doubly linked list of all of our IR nodes. This allows us to walk forward or backward over the IR and they must be ordered correctly to ensure dominance of SSA values.
* Contains `OrderedNodeHeader`
	* Contains `OpNodeWrapper Value`
		* Points to the `IROp_Header` backing op for this SSA node
	* Contains `OrderedNodeWrapper Next`
		* Points to the next `OrderedNode`
	* Contains `OrderedNodeWrapper Previous`
		* Points to the previous `OrderedNode`
* Contains the NumUses
	* This allows us to easily walk to the list backwards and DCE the ops that have NumUses == 0
* `IROp_Header *Op(uintptr_t Base)`
	* Allows you to get the backing IR data for this SSA value

### NodeWrapperBase<typename Type> - Type for `OrderedNodeHeader` and `OpNodeWrapper`
* `using OpNodeWrapper = NodeWrapperBase<IROp_Header>`
* `using OrderedNodeWrapper = NodeWrapperBase<OrderedNode>`
* This is a class to let you more easily convert NodeOffsets in to their real backing pointer
* `GetNode(uintptr_t Base)` allows you to pass in the base pointer from the backing Intrusive allocator and get the object
	* **This can be confusing**
	* A good rule of thumb is to only ever use `GetNode(ListDataBegin)` with OrderedNodeWrapper
	* Then once you have the `OrderedNode*` from GetNode, Use the `Op(IRDataBegin)` function to get the IR data.
	* I do **NOT** recommend using `GetNode` directly from `OpNodeWrapper` as it is VERY easy to mess it up

### NodeIterator
Provides a fairly straightforward interface that allows easily walking the IR nodes with C++ increment and decrement operations.
Only iterates over a single block

#### Example usage
```cpp
	IR::NodeIterator After = ...;
	IR::NodeIterator End = ...;

	while (After != End) {
		// NodeIterator() returns a pair of pointers to the OrderedNode and IROp data
		// You can unpack the result with structured bindings
		auto [CodeNode, IROp] = After();

		// IROp_Header contains a bunch of information about the IR object
		// We can convert it with the object's C<typename Type> or CW<typename Type> functions

		switch(IROp->Op) {
			case IR::OP_ADD: {
				FEXCore::IR::IROp_Add const *Op = IROp->C<FEXCore::IR::IROp_Add>();
				/* We can now access members inside of IROp_Add that were previously unavailable
					 You can still access the header definitions from Op->Header */
				break;
			}
			/* ... */
		}
		// Go to the next IR Op
		++After;
	}

```

### AllNodesIterator
This is like NodeIterator, except that it will cross block boundaries.

### IRListView.GetBlocks()
Provides a range for easy iterating over all the blocks in a multi-block with NodeIterator

#### Example usage
```c++
	for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
		// Do stuff for each block
	}
```

### IRListView.GetCode(BlockNode)
Provides a range for easy iterating over all the code in a block

#### Example usage
```c++
	for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
		// Do stuff for each op

		switch(IROp->Op) {
			case IR::OP_ADD: {
				FEXCore::IR::IROp_Add const *Op = IROp->C<FEXCore::IR::IROp_Add>();
				// Do stuff for each Add op.

				break;
			}
		}
	}
```

### IRListView.GetAllCode()
Like GetCode, except it uses AllNodesIterator to allow easy iterating over every single op in the entire Multiblock

#### Example usage
```c++
	for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
		// Do stuff for each op
	}
```

## JSON file
An example of what the IR json looks like
```
"StoreContext": {
  "SSAArgs": "1",
  "Args": [
    "uint8_t", "Size",
    "uint32_t", "Offset"
  ]
},
```
The json entry name will be the name of the IR op and the dispatcher function.
This means you'll get a `_Add(...)` dispatcher function generated

### JSON IR element options
* `HasDest`
  * This is used on ops that return a value. Used for tracking of if ops return data
* `SSAArgs`
  * These are the number of arguments that the op consumes that are SSA based
  * Needs to come from previous ops that had a destination
* `SSANames`
  * Allows you to name the SSA arguments in an op
  * Otherwise the Op names will only be able to be accessed from the Header of the IR through its arguments array
* `Args`
  * These are defined arguments that are stored in the IR encoding that aren't SSA based
  * Useful for things that are constant encoded and won't change after the fact
* `FixedDestSize`
  * This allows you to override the op's destination size in bytes
  * Most ops with implicitly calculate their destination size through the maximum sizes of the IR arguments passed in
* `DestSize`
  * This allows an IR size override that isn't just a size in bytes
  * This can let the size of the op be another argument or something more extensive
* `RAOverride`
  * This allows an op to take regular SSA arguments (So optimization passes will still be aware of them) but also not have them be register allocated
  * Useful for block handling ops, where blocks aren't something that get register allocated but still need to have their uses tracked
* `HelperGen`
  * If there is a complex IR Op that needs to be defined but you don't want an automatic dispatcher generated then this disables the generation of the
    dispatcher
* `Last`
  * This is a special element only used for the last element in the list
