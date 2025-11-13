#!/usr/bin/python3
from dataclasses import dataclass
import math
import sys
import logging
logger = logging.getLogger()
logger.setLevel(logging.WARNING)

# Usage of this script is `Scripts/GenerateSyscallNumbers.py <Path to Linux directory>`
# This will then parse the syscall headers and format them in an enum
# Then this will be output in stdout
# This output should then be checked and copied to the following headers, splitting up the enums:
#   - Source/Tests/LinuxSyscalls/x32/SyscallsEnum.h
#   - Source/Tests/LinuxSyscalls/x64/SyscallsEnum.h
#   - Source/Tests/LinuxSyscalls/Arm64/SyscallsEnum.h
# `FEX_Syscalls_Common` is provided in the output as just an indicator for which syscalls are using the common
# syscall interface.

@dataclass
class SyscallDefinition:
    arch: str
    syscall_number: int
    abi: str
    name: str
    entry: str
    def __init__(self, Arch, SyscallNumber, ABI, Name, Entry):
        self.arch = Arch
        self.syscall_number = SyscallNumber
        self.abi = ABI
        self.name = Name
        self.entry = Entry

    @property
    def Arch(self):
        return self.arch

    @property
    def Number(self):
        return self.syscall_number

    @property
    def ABI(self):
        return self.abi

    @property
    def Name(self):
        return self.name

    @property
    def EntryName(self):
        return self.entry

Syscallx64File = "/arch/x86/entry/syscalls/syscall_64.tbl"
Syscallx86File = "/arch/x86/entry/syscalls/syscall_32.tbl"
SyscallArm64File = "/include/uapi/asm-generic/unistd.h"

# Syscall names that had naming conflict with some global definitions
# Renamed to work around that issue
DefinitionRenameDict = {
    "pread64": "pread_64",
    "pwrite64": "pwrite_64",
    "prlimit64": "prlimit_64",
    # musl/Alpine Linux defines `fstatat64` as a define that points to `fstatat`.
    # Rename it to avoid global define conflicts.
    "fstatat64": "fstatat_64",
}

Definitions_x64 = []
Definitions_x64_dict = {}
Definitions_x86 = []
Definitions_x86_dict = {}
Definitions_Arm64 = []
Definitions_Arm64_dict = {}

NumArches = 0
SyscallDefinitions = {}

def ParseArchSyscalls(Defs, DefsDict, Arch, FilePath, IgnoreArch):
    global NumArches
    global SyscallDefinitions
    syscall_file = open(FilePath, "r")
    text_lines = syscall_file.readlines()
    syscall_file.close()

    NumArches += 1
    for line in text_lines:
        line = line.strip()

        # Skip lines that are a comment
        if line.startswith("#") or len(line) == 0:
            continue

        # Format: <Number> <ABI> <Name> <Entry Name>
        split_text = line.split()

        Num = split_text[0]
        ABI = split_text[1]

        # If the ABI is on the ignore list then don't store it
        if ABI in IgnoreArch:
            continue

        Name = split_text[2]
        if (len(split_text) < 4):
            # This sometimes happens if the host doesn't have the entry
            EntryName = "<None>"
        else:
            EntryName = split_text[3]

        if Name in DefinitionRenameDict:
            Name = DefinitionRenameDict[Name]

        Def = SyscallDefinition(Arch, Num, ABI, Name, EntryName)

        Defs.append(Def)
        if not Name in SyscallDefinitions:
            SyscallDefinitions[Name] = []

        SyscallDefinitions[Name].append(Def)

def ParseCommonArchSyscalls(Defs, DefsDict, Arch, FilePath):
    global NumArches
    global SyscallDefinitions
    syscall_file = open(FilePath, "r")
    text_lines = syscall_file.readlines()
    syscall_file.close()

    NumArches += 1
    SyscallNumbers = {}
    for line in text_lines:
        line = line.strip()

        if len(line) == 0:
            continue

        # Check for NR defines
        if (line.startswith("#define __NR_") or
           line.startswith("#define __NR3264_")):
            # This line is defining a syscall for us
            # eg: #define __NR_io_setup 0
            line = line.removeprefix("#define __NR_")
            line = line.removeprefix("#define __NR3264_")
            split_text = line.split(" ")

            # Store this for later
            Name = split_text[0]

            # Need to do len here since some lines are multiple spaces between define name and value
            SyscallNumbers[Name] = split_text[len(split_text) - 1]
            continue

        BeginsString = ""
        # Check for __SC_COMP and __SYSCALL defines
        if line.startswith("__SYSCALL("):
            BeginsString = "__SYSCALL("
        elif line.startswith("__SC_COMP("):
            BeginsString = "__SC_COMP("
        elif line.startswith("__SC_3264("):
            BeginsString = "__SC_3264("
        elif line.startswith("__SC_COMP_3264("):
            BeginsString = "__SC_COMP_3264("
        else:
            continue

        line = line.removeprefix(BeginsString)

        if line.startswith("__NR_"):
            BeginsString = "__NR_"
        elif line.startswith("__NR3264_"):
            BeginsString = "__NR3264_"

        line = line.removeprefix(BeginsString)

        split_text = line.split(",")

        Name = split_text[0]
        Num = SyscallNumbers[Name]
        ABI = Arch
        EntryName = split_text[1].strip().split(")")[0]

        if Name in DefinitionRenameDict:
            Name = DefinitionRenameDict[Name]

        Def = SyscallDefinition(Arch, Num, ABI, Name, EntryName)

        Defs.append(Def)
        if not Name in SyscallDefinitions:
            SyscallDefinitions[Name] = []

        SyscallDefinitions[Name].append(Def)

def ExportSyscallDefines(Defs, DefsDict, Arch, UnsupportedDefs):
    AlreadyExported = []

    print("enum Syscalls_{} {{".format(Arch))
    for Def in Defs:
        if Def.EntryName == "<None>":
            print("  // No entrypoint. -ENOSYS")
        print("  SYSCALL_{}_{} = {},".format(Arch, Def.Name, Def.Number))
        AlreadyExported.append(Def.Name)

    # Print ourselves a max
    Max = 1 << (int(math.log(len(Defs), 2)) + 1)
    print("  SYSCALL_{}_MAX = {},".format(Arch, Max))

    if len(UnsupportedDefs) != 0:
        # Print out syscalls that don't exist on this architecture
        print("")
        print("  // Unsupported syscalls on this host")

        for DefList in UnsupportedDefs:
            for Def in DefList:
                # If the syscall name exists in the full definition dictionary
                # but DOESN'T exist in our current arch AND exists in the Unsupported dicts
                # Then we need to export it as an unnamed syscall entry
                if Def.Name in AlreadyExported:
                    continue

                print("  SYSCALL_{}_{} = ~0,".format(Arch, Def.Name))

                AlreadyExported.append(Def.name)

    print("};")

def ExportCommonSyscallDefines():
    global Definitions_Arm64
    global SyscallDefinitions

    print("enum FEX_Syscalls_Common {")
    for Def in Definitions_Arm64:
        # Check the dict to ensure the definitions exist everywhere
        if not Def.Name in SyscallDefinitions:
            continue

        Defs = SyscallDefinitions[Def.Name]
        if len(Defs) != NumArches:
            continue

        Number = Def.Number
        Matches = True
        for AllDef in Defs:
            if AllDef.Number != Def.Number:
                Matches = False

        if not Matches:
            continue

        for AllDef in Defs:
            if AllDef.EntryName == "<None>":
                print("  // {} No entrypoint. -ENOSYS".format(AllDef.Arch))

        print("  SYS_{} = {},".format(Def.Name, Def.Number))


    # Find a max between all architectures
    Maximums = []
    for Defs in [Definitions_x64, Definitions_x86, Definitions_Arm64]:
        Maximums.append(1 << (int(math.log(len(Defs), 2)) + 1))
    print("  SYSCALL_MAX = {},".format(max(Maximums)))

    print("};")


def main():
    if sys.version_info[0] < 3:
        logging.critical ("Python 3 or a more recent version is required.")

    if (len(sys.argv) < 2):
        print ("usage: %s <Linux git tree>" % (sys.argv[0]))

    LinuxPath = sys.argv[1]


    ParseArchSyscalls(Definitions_x86, Definitions_x86_dict, "x86", LinuxPath + Syscallx86File, [])
    ParseArchSyscalls(Definitions_x64, Definitions_x64_dict, "x64", LinuxPath + Syscallx64File, ["x32"])
    ParseCommonArchSyscalls(Definitions_Arm64, Definitions_Arm64_dict, "Arm64", LinuxPath + SyscallArm64File)

    ExportSyscallDefines(Definitions_x86, Definitions_x86_dict, "x86",[])
    ExportSyscallDefines(Definitions_x64, Definitions_x64_dict, "x64", [Definitions_x86])
    ExportSyscallDefines(Definitions_Arm64, Definitions_Arm64_dict, "Arm64", [Definitions_x86, Definitions_x64])

    ExportCommonSyscallDefines()

if __name__ == "__main__":
    # execute only if run as a script
    sys.exit(main())
