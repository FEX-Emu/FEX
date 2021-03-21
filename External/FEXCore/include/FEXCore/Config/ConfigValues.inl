#ifndef OPT_BASE
#define OPT_BASE(type, group, enum, json, env, default)
#endif
#ifndef OPT_BOOL
#define OPT_BOOL(group, enum, json, env, default) OPT_BASE(bool, group, enum, json, env, default)
#endif
#ifndef OPT_UINT8
#define OPT_UINT8(group, enum, json, env, default) OPT_BASE(uint8_t, group, enum, json, env, default)
#endif
#ifndef OPT_INT32
#define OPT_INT32(group, enum, json, env, default) OPT_BASE(int32_t, group, enum, json, env, default)
#endif
#ifndef OPT_UINT32
#define OPT_UINT32(group, enum, json, env, default) OPT_BASE(uint32_t, group, enum, json, env, default)
#endif
#ifndef OPT_UINT64
#define OPT_UINT64(group, enum, json, env, default) OPT_BASE(uint64_t, group, enum, json, env, default)
#endif
#ifndef OPT_STR
#define OPT_STR(group, enum, json, env, default) OPT_BASE(std::string, group, enum, json, env, default)
#endif
#ifndef OPT_STRARRAY
#define OPT_STRARRAY(group, enum, json, env, default) OPT_BASE(std::string, group, enum, json, env, default)
#endif

// Option definitions
// Arguments
// 1) Group
// 2) Enum name
// 3) json Option name
// 4) Environment Option name
// 5) Default option
OPT_UINT32(CPU, DEFAULTCORE,      Core,       CORE,       FEXCore::Config::ConfigCore::CONFIG_IRJIT)
OPT_BOOL  (CPU, MULTIBLOCK,       Multiblock, MULTIBLOCK, true)
OPT_INT32 (CPU, MAXBLOCKINST,     MaxInst,    MAXINST,    5000)
OPT_INT32 (CPU, EMULATED_CPU_CORES, Threads,  THREADS,    1)

OPT_STR   (EMULATION, ROOTFSPATH,    RootFS,     ROOTFS,     "")
OPT_STR   (EMULATION, THUNKHOSTLIBSPATH, ThunkHostLibs,  THUNKHOSTLIBS,  "")
OPT_STR   (EMULATION, THUNKGUESTLIBSPATH, ThunkGuestLibs,  THUNKGUESTLIBS,  "")
OPT_STR   (EMULATION, THUNKCONFIGPATH, ThunkConfig, THUNKCONFIG,  "")

OPT_STRARRAY (EMULATION, ENVIRONMENT,   Env,        ENV,        "")

OPT_BOOL  (DEBUG, SINGLESTEP,   SingleStep, SINGLESTEP, false)
OPT_BOOL  (DEBUG, GDBSERVER,    GdbServer,  GDBSERVER,  false)
OPT_STR   (DEBUG, DUMPIR,       DumpIR,     DUMPIR,     "no")
OPT_BOOL  (DEBUG, DUMP_GPRS,    DumpGPRs,   DUMP_GPRS,  false)
OPT_BOOL  (DEBUG, DEBUG_DISABLE_OPTIMIZATION_PASSES, O0, O0, false)

OPT_BOOL (LOGGING, SILENTLOGS, SilentLog, SILENTLOG, false)
OPT_STR  (LOGGING, OUTPUTLOG,  OutputLog, OUTPUTLOG, "")

OPT_BOOL  (HACKS, TSO_ENABLED,     TSOEnabled, TSO_ENABLED,      true)
OPT_UINT8 (HACKS, SMC_CHECKS,      SMCChecks,  SMC_CHECKS,       FEXCore::Config::CONFIG_SMC_MMAN)
OPT_BOOL  (HACKS, ABI_LOCAL_FLAGS, ABILocalFlags, ABILOCALFLAGS, false)
OPT_BOOL  (HACKS, ABI_NO_PF,       ABINoPF,       ABINOPF,       false)

OPT_BOOL (MISC, IS_INTERPRETER, INVALID, INVALID, false)
OPT_BOOL (MISC, INTERPRETER_INSTALLED, INVALID, INVALID, false)
OPT_STR  (MISC, APP_FILENAME, INVALID, INVALID, "")
OPT_BOOL (MISC, AOTIR_GENERATE, AOTIRCapture, AOT_GENERATE, false)
OPT_BOOL (MISC, AOTIR_LOAD, AOTIRLoad, AOT_LOAD, false)
OPT_BOOL (MISC, IS64BIT_MODE, INVALID, INVALID, false)

#undef OPT_BASE
#undef OPT_BOOL
#undef OPT_UINT8
#undef OPT_INT32
#undef OPT_UINT32
#undef OPT_UINT64
#undef OPT_STR
#undef OPT_STRARRAY
