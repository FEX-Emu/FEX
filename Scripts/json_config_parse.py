from enum import Flag
import json
import struct
import sys

class Regs(Flag):
    REG_NONE  = 0
    REG_RIP   = (1 << 0)
    REG_RAX   = (1 << 1)
    REG_RBX   = (1 << 2)
    REG_RCX   = (1 << 3)
    REG_RDX   = (1 << 4)
    REG_RSI   = (1 << 5)
    REG_RDI   = (1 << 6)
    REG_RBP   = (1 << 7)
    REG_RSP   = (1 << 8)
    REG_R8    = (1 << 9)
    REG_R9    = (1 << 10)
    REG_R10   = (1 << 11)
    REG_R11   = (1 << 12)
    REG_R12   = (1 << 13)
    REG_R13   = (1 << 14)
    REG_R14   = (1 << 15)
    REG_R15   = (1 << 16)
    REG_XMM0  = (1 << 17)
    REG_XMM1  = (1 << 18)
    REG_XMM2  = (1 << 19)
    REG_XMM3  = (1 << 20)
    REG_XMM4  = (1 << 21)
    REG_XMM5  = (1 << 22)
    REG_XMM6  = (1 << 23)
    REG_XMM7  = (1 << 24)
    REG_XMM8  = (1 << 25)
    REG_XMM9  = (1 << 26)
    REG_XMM10 = (1 << 27)
    REG_XMM11 = (1 << 28)
    REG_XMM12 = (1 << 29)
    REG_XMM13 = (1 << 30)
    REG_XMM14 = (1 << 31)
    REG_XMM15 = (1 << 32)
    REG_GS    = (1 << 33)
    REG_FS    = (1 << 34)
    REG_FLAGS = (1 << 35)
    REG_MM0   = (1 << 36)
    REG_MM1   = (1 << 37)
    REG_MM2   = (1 << 38)
    REG_MM3   = (1 << 39)
    REG_MM4   = (1 << 40)
    REG_MM5   = (1 << 41)
    REG_MM6   = (1 << 42)
    REG_MM7   = (1 << 43)
    REG_MM8   = (1 << 44)
    REG_ALL   = (1 << 45) - 1
    REG_INVALID = (1 << 45)

class ABI(Flag) :
    ABI_SYSTEMV = 0
    ABI_WIN64   = 1
    ABI_NONE    = 2

class Mode(Flag) :
    MODE_32   = 0
    MODE_64   = 1

RegStringLookup = {
    "NONE":  Regs.REG_NONE,
    "RAX":   Regs.REG_RAX,
    "RIP":   Regs.REG_RIP,
    "RBX":   Regs.REG_RBX,
    "RCX":   Regs.REG_RCX,
    "RDX":   Regs.REG_RDX,
    "RSI":   Regs.REG_RSI,
    "RDI":   Regs.REG_RDI,
    "RBP":   Regs.REG_RBP,
    "RSP":   Regs.REG_RSP,
    "R8":    Regs.REG_R8,
    "R9":    Regs.REG_R9,
    "R10":   Regs.REG_R10,
    "R11":   Regs.REG_R11,
    "R12":   Regs.REG_R12,
    "R13":   Regs.REG_R13,
    "R14":   Regs.REG_R14,
    "R15":   Regs.REG_R15,
    "XMM0":  Regs.REG_XMM0,
    "XMM1":  Regs.REG_XMM1,
    "XMM2":  Regs.REG_XMM2,
    "XMM3":  Regs.REG_XMM3,
    "XMM4":  Regs.REG_XMM4,
    "XMM5":  Regs.REG_XMM5,
    "XMM6":  Regs.REG_XMM6,
    "XMM7":  Regs.REG_XMM7,
    "XMM8":  Regs.REG_XMM8,
    "XMM9":  Regs.REG_XMM9,
    "XMM10": Regs.REG_XMM10,
    "XMM11": Regs.REG_XMM11,
    "XMM12": Regs.REG_XMM12,
    "XMM13": Regs.REG_XMM13,
    "XMM14": Regs.REG_XMM14,
    "XMM15": Regs.REG_XMM15,
    "GS":    Regs.REG_GS,
    "FS":    Regs.REG_FS,
    "FLAGS": Regs.REG_FLAGS,
    "ALL":   Regs.REG_ALL,
    "MM0":   Regs.REG_MM0,
    "MM1":   Regs.REG_MM1,
    "MM2":   Regs.REG_MM2,
    "MM3":   Regs.REG_MM3,
    "MM4":   Regs.REG_MM4,
    "MM5":   Regs.REG_MM5,
    "MM6":   Regs.REG_MM6,
    "MM7":   Regs.REG_MM7,
    "MM8":   Regs.REG_MM8,
}

ABIStringLookup = {
    "SYSTEMV": ABI.ABI_SYSTEMV,
    "WIN64": ABI.ABI_WIN64,
    "NONE": ABI.ABI_NONE,
}

ModeStringLookup = {
    "32BIT": Mode.MODE_32,
    "64BIT": Mode.MODE_64,
}

def parse_hexstring(s):
    length = 0
    byte_data = []
    for num in s.split(' '):
        if s.startswith("0x"):
            num = num[2:]
        while len(num) > 0:
            byte_num = num[-2:]
            byte_data.append(int(byte_num, 16))
            length += 1
            num = num[0:-2]
    return length, byte_data


def parse_json(json_text, output_file):
    # Default options
    OptionMatch = Regs.REG_INVALID
    OptionIgnore = Regs.REG_NONE
    OptionABI = ABI.ABI_SYSTEMV
    OptionMode = Mode.MODE_64
    OptionStackSize = 4096
    OptionEntryPoint = 1
    OptionRegData = {}
    OptionMemoryRegions = {}
    OptionMemoryData = {}


    json_object = json.loads(json_text)
    json_object = {k.upper(): v for k, v in json_object.items()}

    # Begin parsing the JSON
    if ("MATCH" in json_object):
        data = json_object["MATCH"]
        if (type(data) is str):
            data = [data]

        for data_val in data:
            data_val = data_val.upper()
            if not (data_val in RegStringLookup):
                sys.exit("Invalid Match register option")
            if (OptionMatch == Regs.REG_INVALID):
                OptionMatch = Regs.REG_NONE
            RegOption = RegStringLookup[data_val]
            OptionMatch = OptionMatch | RegOption

    if ("IGNORE" in json_object):
        data = json_object["IGNORE"]
        if (type(data) is str):
            data = [data]

        for data_val in data:
            data_val = data_val.upper()
            if not (data_val in RegStringLookup):
                sys.exit("Invalid Ignore register option")
            if (OptionMatch == Regs.REG_INVALID):
                OptionMatch = Regs.REG_NONE
            RegOption = RegStringLookup[data_val]
            OptionIgnore = OptionIgnore | RegOption

    if ("ABI" in json_object):
        data = json_object["ABI"]
        data = data.upper()
        if not (data in ABIStringLookup):
            sys.exit("Invalid ABI")
        OptionABI = ABIStringLookup[data]

    if ("MODE" in json_object):
        data = json_object["MODE"]
        data = data.upper()
        if not (data in ModeStringLookup):
            sys.exit("Invalid Mode")
        OptionMode = ModeStringLookup[data]

    if ("STACKSIZE" in json_object):
        data = json_object["STACKSIZE"]
        OptionStackSize = int(data, 0)

    if ("ENTRYPOINT" in json_object):
        data = json_object["ENTRYPOINT"]
        data = int(data, 0)
        if (data == 0):
            sys.exit("Invalid entrypoint of 0")
        OptionEntryPoint = data

    if ("MEMORYREGIONS" in json_object):
        data = json_object["MEMORYREGIONS"]
        if not (type(data) is dict):
            sys.exit("MemoryRegions value must be list of key:value pairs")
        for data_key, data_val in data.items():
            OptionMemoryRegions[int(data_key, 0)] = int(data_val, 0)

    if ("REGDATA" in json_object):
        data = json_object["REGDATA"]
        if not (type(data) is dict):
            sys.exit("RegData value must be list of key:value pairs")
        for data_key, data_val in data.items():
            data_key = data_key.upper()
            if not (data_key in RegStringLookup):
                sys.exit("Invalid RegData register option")

            data_key_index = RegStringLookup[data_key]
            data_key_values = []

            # Create a list of values for this register as an integer
            if (type(data_val) is list):
                for data_key_value in data_val:
                    data_key_values.append(int(data_key_value, 0))
            else:
                data_key_values.append(int(data_val, 0))
            OptionRegData[data_key_index] = data_key_values

    if ("MEMORYDATA" in json_object):
        data = json_object["MEMORYDATA"]
        if not (type(data) is dict):
            sys.exit("MemoryData value must be list of key:value pairs")
        for data_key, data_val in data.items():
            length, byte_data = parse_hexstring(data_val)
            OptionMemoryData[int(data_key, 0)] = (length, byte_data)

    # If Match option wasn't touched then set it to the default
    if (OptionMatch == Regs.REG_INVALID):
        OptionMatch = Regs.REG_NONE


    memRegions = bytes()
    regData = bytes()
    memData = bytes()

    # Write memory regions
    for key, val in OptionMemoryRegions.items():
        memRegions += struct.pack('Q', key)
        memRegions += struct.pack('Q', val)

    # Write Register values
    for reg_key, reg_val in OptionRegData.items():
        regData += struct.pack('I', len(reg_val))
        regData += struct.pack('Q', reg_key.value)
        for val in reg_val:
            regData += struct.pack('Q', val)

    # Write Memory data
    for reg_key, reg_val in OptionMemoryData.items():
        length, data = reg_val
        memData += struct.pack('Q', reg_key) # address
        memData += struct.pack('I', length)
        for byte in data:
            memData += struct.pack('B', byte)

    config_file = open(output_file, "wb")
    config_file.write(struct.pack('Q', OptionMatch.value))
    config_file.write(struct.pack('Q', OptionIgnore.value))
    config_file.write(struct.pack('Q', OptionStackSize))
    config_file.write(struct.pack('Q', OptionEntryPoint))
    config_file.write(struct.pack('I', OptionABI.value))
    config_file.write(struct.pack('I', OptionMode.value))

    # Total length of header, including offsets/counts below
    headerLength = (8 * 4) + (4 * 2) + (4 * 6)
    offset = headerLength

    #  memory regions offset/count
    config_file.write(struct.pack('I', offset))
    config_file.write(struct.pack('I', len(OptionMemoryRegions)))
    offset += len(memRegions)

    # register values offset/count
    config_file.write(struct.pack('I', offset))
    config_file.write(struct.pack('I', len(OptionRegData)))
    offset += len(regData)

    # memory data offset/count
    config_file.write(struct.pack('I', offset))
    config_file.write(struct.pack('I', len(OptionMemoryData)))
    offset += len(memData)

    # write out the actual data for memory regions, reg data and memory data
    config_file.write(memRegions)
    config_file.write(regData)
    config_file.write(memData)

    config_file.close()

