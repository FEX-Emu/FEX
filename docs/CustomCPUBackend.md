# FEXCore custom CPU backends
---
Custom CPU backends can be useful for testing purposes or wanting to support situations that FEXCore doesn't currently understand.
The FEXCore::Context namespace provides a `SetCustomCPUBackendFactory` function for providing a factory function pointer to the core. This function will be used if the `DEFAULTCORE` configuration option is set to `CUSTOM`.
If the guest code creates more threads then the CPU factory function will be invoked for creating a CPUBackend per thread. If you don't want a unique CPUBackend object per thread then that needs to be handled by the user.

It's recommended to store the pointers provided to the factory function for later use.
`FEXCore::Context::Context*` - Is a pointer to previously generated context object
`FEXCore::Core::ThreadState*` - Is a pointer to a thread's state. Lives for as long as the guest thread is alive.
To use this factory, one must override the provided `FEXCore::CPU::CPUBackend` class with a custom one. This factory function then should return a newly allocated class.

`FEXCore::CPU::CPUBackend::GetName` - Returns an `std::string` for the name of this core
`FEXCore::CPU::CPUBackend::CompileCode` - Provides the CPUBackend with potentially an IR and DebugData for compiling code. Returns a pointer that needs to be long lasting to a piece of code that will be executed for the particular RIP.
Both IR and DebugData can be null if `NeedsOpDispatch` returns false
`FEXCore::CPU::CPUBackend::MapRegion` - This function needs to be implemented if the CPUBackend needs to map host facing memory in to the backend. Allows setting up virtual memory mapping if required
`FEXCore::CPU::CPUBackend::Initialize` - Called after the guest memory is initialized and all state is ready for the code to start initializing. Gets called just before the CPUBackend starts executing code for the first time.
`FEXCore::CPU::CPUBackend::NeedsOpDispatch` - Tells FEXCore if the backend needs the FEXCore IR and DebugData provided to it. This can be useful if FEXCore hits something it doesn't understand but it doesn't matter since the CPUBackend can still understand it from raw x86-64 (ex VM based CPU backend).
