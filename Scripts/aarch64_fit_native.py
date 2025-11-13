#!/usr/bin/python3
import re
import sys
try:
    from packaging.version import Version as version_check
except:
    from pkg_resources import parse_version as version_check

# Order this list from oldest to newest
# try not to list something newer than our minimum compiler supported version
BigCoreIDs = {
        # ARM
        tuple([0x41, 0xd07]): "cortex-a57",
        tuple([0x41, 0xd08]): "cortex-a72",
        tuple([0x41, 0xd09]): "cortex-a73",
        tuple([0x41, 0xd0a]): "cortex-a75",
        tuple([0x41, 0xd0b]): "cortex-a76",
        tuple([0x41, 0xd0d]): "cortex-a77",
        tuple([0x41, 0xd41]): "cortex-a78",
        tuple([0x41, 0xd4b]): "cortex-a78c",
        tuple([0x41, 0xd44]): "cortex-x1",
        tuple([0x41, 0xd4c]):
            [ ["cortex-x1", "0.0"],
              ["cortex-x1c", "14.0"],
            ],
        tuple([0x41, 0xd47]):
            [ ["cortex-a78", "0.0"],
              ["cortex-a710", "14.0"],
            ],
        tuple([0x41, 0xd48]):
            [ ["cortex-x1", "0.0"],
              ["cortex-x2", "14.0"],
            ],
        tuple([0x41, 0xd4d]):
            [ ["cortex-a78", "0.0"],
              ["cortex-a715", "17.0"],
            ],
        tuple([0x41, 0xd81]):
            [ ["cortex-a78", "0.0"],
              ["cortex-a720", "18.0"],
            ],
        tuple([0x41, 0xd87]):
            [ ["cortex-a78", "0.0"],
              ["cortex-a725", "19.0"],
            ],
        tuple([0x41, 0xd85]):
            [ ["cortex-a78", "0.0"],
              ["cortex-x925", "19.0"],
            ],
        # Neoverse-N class
        tuple([0x41, 0xd0c]): "neoverse-n1",
        tuple([0x41, 0xd49]): "neoverse-n2",
        tuple([0x41, 0xd8e]):
            [ ["cortex-a78", "0.0"],
              ["neoverse-n3", "19.0"],
            ],

        # Neoverse-V class
        tuple([0x41, 0xd40]): "neoverse-v1",
        tuple([0x41, 0xd4f]):
            [ ["cortex-a78", "0.0"],
              ["neoverse-v2", "16.0"],
            ],
        tuple([0x41, 0xd83]):
            [ ["cortex-a78", "0.0"],
              ["neoverse-v3ae", "19.0"],
            ],
        tuple([0x41, 0xd84]):
            [ ["cortex-a78", "0.0"],
              ["neoverse-v3", "19.0"],
            ],
        ## Nvidia
        tuple([0x4e, 0x004]): "carmel", # Carmel
        # Qualcomm
        tuple([0x51, 0x800]): "cortex-a73", # Kryo 2xx Gold
        tuple([0x51, 0x802]): "cortex-a75", # Kryo 3xx Gold
        tuple([0x51, 0x804]): "cortex-a76", # Kryo 4xx Gold
        # Apple M1 Parallels hypervisor
        tuple([0x41, 0x0]):
            [ ["apple-a13", "0.0"], # If we aren't on 12.0+
              ["apple-a14", "12.0"], # Only exists in 12.0+
            ],
        # QEmu HVF 10.2+
        tuple([0x61, 0]): "apple-a13", # Can't determine variant, choose lowest.
}

LittleCoreIDs = {
        # ARM
        tuple([0x41, 0xd04]): "cortex-a35",
        tuple([0x41, 0xd03]): "cortex-a53",
        tuple([0x41, 0xd05]): "cortex-a55",
        tuple([0x41, 0xd46]):
            [ ["cortex-a55", "0.0"],
              ["cortex-a510", "14.0"],
            ],
        tuple([0x41, 0xd80]):
            [ ["cortex-a55", "0.0"],
              ["cortex-a520", "18.0"],
            ],
        # Qualcomm
        tuple([0x51, 0x801]): "cortex-a53", # Kryo 2xx Silver
        tuple([0x51, 0x803]): "cortex-a55", # Kryo 3xx Silver
        tuple([0x51, 0x805]): "cortex-a55", # Kryo 4xx/5xx Silver
}

# Args: </proc/cpuinfo file> <clang version>
if (len(sys.argv) < 3):
    sys.exit()

clang_version = sys.argv[2]
cpuinfo = []
with open(sys.argv[1]) as cpuinfo_file:
    current_implementer = 0
    current_part = 0
    for line in cpuinfo_file:
        line = line.strip()
        if "CPU implementer" in line:
            current_implementer = int(re.findall(r'0x[0-9A-F]+', line, re.I)[0], 16)
        if "CPU part" in line:
            current_part = int(re.findall(r'0x[0-9A-F]+', line, re.I)[0], 16)
            cpuinfo += {tuple([current_implementer, current_part])}

largest_big = "cortex-a57"
largest_little = "cortex-a53"

for core in cpuinfo:
    if BigCoreIDs.get(core):
        IDList = BigCoreIDs.get(core)
        if type(IDList) is list:
            for ID in IDList:
                if version_check(clang_version) >= version_check(ID[1]):
                    largest_big = ID[0]
        else:
            largest_big = BigCoreIDs.get(core)

    if LittleCoreIDs.get(core):
        largest_little = LittleCoreIDs.get(core)

# We only want the big core output
print(largest_big)
# print(largest_little)
