#!/usr/bin/python3
from dataclasses import dataclass, field
import json
import struct
import sys
from json_config_parse import parse_json
import logging
logger = logging.getLogger()
logger.setLevel(logging.WARNING)

@dataclass
class SyscallDefinition:
    Name: str
    CustomHandler: bool
    HandlerType: str
    ArgCount: int
    Flags: str
    MinimumKernel: str
    SyscallRedirect: str

    def __init__(self, Name, CustomHandler, HandlerType, ArgCount, Flags, MinimumKernel, SyscallRedirect):
        self.Name = Name
        self.CustomHandler = CustomHandler
        self.HandlerType = HandlerType
        self.ArgCount = ArgCount
        self.Flags = Flags
        self.MinimumKernel = MinimumKernel
        self.SyscallRedirect = SyscallRedirect

SyscallDefinitionsCommon = {}
SyscallDefinitionsx64 = {}
SyscallDefinitionsx32 = {}

def ParseTable(json_object, table, name):
    data = json_object[name]
    for data_key, data_val in data.items():
        name = data_key

        argcount = 0
        flags = ""
        MinimumKernel = ""
        SyscallRedirect = ""
        CustomHandler = False

        if "CustomHandler" in data_val:
            CustomHandler = bool(data_val["CustomHandler"])

        if "ArchSpecific" in data_val:
            if bool(data_val["ArchSpecific"]):
                continue

        if not CustomHandler:
            if not "ArgCount" in data_val:
                logging.critical("Syscall {} doesn't have argument count".format(name))

            if not "Flags" in data_val:
                logging.critical("Syscall {} doesn't have flags".format(name))

            argcount = data_val["ArgCount"]
            flags = data_val["Flags"]

        if "MinimumKernel" in data_val:
            MinimumKernel = data_val["MinimumKernel"]

            Splits = MinimumKernel.split(".")
            if len(Splits) != 3:
                logging.critical("Syscall {} has invalid kernel version as '{}'. Expecting format 'Major.Minor.Patch'".format(name, MinimumKernel))

        if "SyscallRedirect" in data_val:
            SyscallRedirect = data_val["SyscallRedirect"]

        table[name] = SyscallDefinition(name, CustomHandler, "Common", argcount, flags, MinimumKernel, SyscallRedirect)

def ParseJson(JsonText):
    json_object = json.loads(JsonText)

    if not "Common" in json_object:
        logging.critical ("Need to have common syscalls")

    if not "x64" in json_object:
        logging.critical ("Need to have x64 syscalls")

    if not "x32" in json_object:
        logging.critical ("Need to have x32 syscalls")

    ParseTable(json_object, SyscallDefinitionsCommon, "Common")
    ParseTable(json_object, SyscallDefinitionsx64, "x64")
    ParseTable(json_object, SyscallDefinitionsx32, "x32")

def PrintHandlers(output_file, wrapper_name, table, impl_flags, impl, redirect_impl_flags, redirect_impl):
    output_file.write("#ifdef {}\n".format(wrapper_name))
    output_file.write("#undef {}\n".format(wrapper_name))
    for data_key, data_val in table.items():
        if data_val.CustomHandler:
            continue

        HasMinimumKernel = data_val.MinimumKernel != ""
        HasSyscallRedirect = data_val.SyscallRedirect != ""

        which_impl_flags = impl_flags
        which_impl = impl

        if HasSyscallRedirect:
            which_impl_flags = redirect_impl_flags
            which_impl = redirect_impl

        if HasMinimumKernel:
            Splits = data_val.MinimumKernel.split(".")
            output_file.write ("if (Handler->IsHostKernelVersionAtLeast({}, {}, {})) {{\n".format(Splits[0], Splits[1], Splits[2]))

        if HasSyscallRedirect:
            output_file.write ("{}({}, {}, {},\n".format(which_impl_flags, data_key, data_val.SyscallRedirect, data_val.Flags))
        else:
            output_file.write ("{}({}, {},\n".format(which_impl_flags, data_key, data_val.Flags))
        output_file.write ("  SyscallPassthrough{}<SYSCALL_DEF({})>);\n".format(data_val.ArgCount, data_key))

        if HasMinimumKernel:
            output_file.write ("}\n")
            output_file.write ("else {\n")

            output_file.write ("  {}({}, UnimplementedSyscallSafe);\n".format(which_impl, data_key))
            output_file.write ("}\n")

    output_file.write("#endif\n")

def PrintPassthroughHandlers(output_file):
    output_file.write("#ifdef PASSTHROUGH_HANDLERS\n")
    output_file.write("#undef PASSTHROUGH_HANDLERS\n")

    output_file.write("#ifdef _M_ARM_64\n")
    for i in range(0, 8):
        output_file.write("template<int syscall_num>\n");
        output_file.write("uint64_t SyscallPassthrough{}(FEXCore::Core::CpuStateFrame *Frame".format(i))

        for j in range(0, i):
            output_file.write(", uint64_t arg{}".format(j + 1))

        output_file.write(") {\n")
        if i > 0:
            for j in range(0, i):
                output_file.write("  register uint64_t x{} asm (\"x{}\") = arg{};\n".format(j, j, j + 1))
        else:
            output_file.write("  register uint64_t x0 asm (\"x0\");\n")

        output_file.write("  register int x8 asm (\"x8\") = syscall_num;\n");
        output_file.write("  __asm volatile(R\"(\n")
        output_file.write("    svc #0;\n")
        output_file.write("  )\"\n")
        output_file.write("  : \"=r\" (x0)\n")
        output_file.write("  : \"r\" (x8)\n")
        for j in range(0, i):
            output_file.write("  , \"r\" (x{})\n".format(j))

        output_file.write("  : \"memory\");\n")

        output_file.write("  return x0;\n")
        output_file.write("}\n\n")

    output_file.write("#else\n")

    for i in range(0, 8):
        output_file.write("template<int syscall_num>\n")
        output_file.write("uint64_t SyscallPassthrough{}(FEXCore::Core::CpuStateFrame *Frame".format(i))

        for j in range(0, i):
            output_file.write(", uint64_t arg{}".format(j + 1))

        output_file.write(") {\n")

        output_file.write("  uint64_t Result = ::syscall(syscall_num")
        for j in range(0, i):
            output_file.write(", arg{}".format(j + 1))

        output_file.write(");\n")

        output_file.write("  SYSCALL_ERRNO();\n")
        output_file.write("}\n")
    output_file.write("#endif\n")
    output_file.write("#endif\n")

def main():
    if sys.version_info[0] < 3:
        logging.critical ("Python 3 or a more recent version is required.")

    if (len(sys.argv) < 3):
        print ("usage: %s <Syscall description json> <output file>" % (sys.argv[0]))

    JsonDescPath = sys.argv[1]
    OutputFilePath = sys.argv[2]

    JsonFile = open(JsonDescPath, "r")
    JsonText = JsonFile.read()
    JsonFile.close()

    ParseJson(JsonText)

    output_file = open(OutputFilePath, "w")
    PrintPassthroughHandlers(output_file)

    PrintHandlers(output_file,
        "SYSCALL_COMMON_IMPL",
        SyscallDefinitionsCommon,
        "REGISTER_SYSCALL_IMPL_PASS_FLAGS", "REGISTER_SYSCALL_IMPL",
        "<UNSUPPORTED>", "<UNSUPPORTED>")
    PrintHandlers(output_file,
        "SYSCALL_X64_IMPL",
        SyscallDefinitionsx64,
        "REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS", "REGISTER_SYSCALL_IMPL_X64",
        "<UNSUPPORTED>", "<UNSUPPORTED>")
    PrintHandlers(output_file,
        "SYSCALL_X32_IMPL",
        SyscallDefinitionsx32,
        "REGISTER_SYSCALL_IMPL_X32_PASS_FLAGS", "REGISTER_SYSCALL_IMPL_X32",
        "REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS", "REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL")

    output_file.close()

if __name__ == "__main__":
# execute only if run as a script
    sys.exit(main())
