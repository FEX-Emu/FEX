#!/usr/bin/python3
import os
import subprocess
import re
import sys
import tempfile
import platform

def ListContainsRequired(Features, RequiredFeatures):
    return RequiredFeatures in Features

def GetCPUFeatures():
    File = open("/proc/cpuinfo", "r")
    Lines = File.readlines()
    File.close()

    # Minimum spec is ARMv8.0
    _ArchVersion = "8.0"
    for Line in Lines:
        if "Features" in Line:
            Features = Line.split(":")[1].strip().split(" ")
            return Features


SnapdragonIDsThatSupportSVE = {
    # Snapdragon 8 Gen 3
    tuple([0x41, 0xd82]): True, # Cortex-X4
    tuple([0x41, 0xd81]): True, # Cortex-A720
    tuple([0x41, 0xd80]): True, # Cortex-A520

    # Snapdragon 8 Gen 2
    tuple([0x41, 0xd4e]): True, # Cortex-X3
    tuple([0x41, 0xd4d]): True, # Cortex-A715
    tuple([0x41, 0xd47]): True, # Cortex-A710
    tuple([0x41, 0xd46]): True, # Cortex-A510

    # Snapdragon 8 Gen 1
    tuple([0x41, 0xd48]): True, # Cortex-X2
    # A710
    # A510
}

def IsAffectedSnapdragon():
    cpuinfo = []

    with open("/proc/cpuinfo") as cpuinfo_file:
        current_implementer = 0
        current_part = 0
        for line in cpuinfo_file:
            line = line.strip()
            if "CPU implementer" in line:
                current_implementer = int(re.findall(r'0x[0-9A-F]+', line, re.I)[0], 16)
            if "CPU part" in line:
                current_part = int(re.findall(r'0x[0-9A-F]+', line, re.I)[0], 16)
                cpuinfo += {tuple([current_implementer, current_part])}

    for core in cpuinfo:
        if SnapdragonIDsThatSupportSVE.get(core):
            return True

    return False


def main():
    Features = GetCPUFeatures()

    # If SVE is reported from cpuinfo just return.
    if ListContainsRequired(Features, "sve"):
        sys.exit(0)

    if IsAffectedSnapdragon():
        sys.exit(1)

    sys.exit(0)

if __name__ == "__main__":
    sys.exit(main())
