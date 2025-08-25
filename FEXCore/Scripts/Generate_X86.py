#!/bin/python3
from enum import Enum, auto
from typing import NamedTuple
import sys

class InstType(Enum):
  UNKNOWN = auto()
  LEGACY_PREFIX = auto()
  PREFIX = auto()
  REX_PREFIX = auto()
  SECONDARY_TABLE_PREFIX = auto()
  X87_TABLE_PREFIX = auto()
  VEX_TABLE_PREFIX = auto()
  INST = auto()
  X87 = INST
  INVALID = auto()
  COPY_OTHER = auto()
  ARCH_DISPATCHER = auto()

  GROUP_1 = auto()
  GROUP_1A = auto()
  GROUP_2 = auto()
  GROUP_3 = auto()
  GROUP_4 = auto()
  GROUP_5 = auto()
  GROUP_11 = auto()

  GROUP_6 = auto()
  GROUP_7 = auto()
  GROUP_8 = auto()
  GROUP_9 = auto()
  GROUP_10 = auto()
  GROUP_12 = auto()
  GROUP_13 = auto()
  GROUP_14 = auto()
  GROUP_15 = auto()
  GROUP_16 = auto()
  GROUP_17 = auto()
  GROUP_P = auto()

  SECOND_GROUP_MODRM = auto()

  VEX_GROUP_12 = auto()
  VEX_GROUP_13 = auto()
  VEX_GROUP_14 = auto()
  VEX_GROUP_15 = auto()
  VEX_GROUP_17 = auto()

  GROUP_EVEX = auto()

  UNDEC = INVALID
  MMX = INVALID
  PRIV = INVALID
  H0F38_TABLE = INVALID
  H0F3A_TABLE = INVALID
  H3DNOW_TABLE = INVALID

class X86InstInfo(NamedTuple):
    name : str
    type : InstType
    flags : str = "FLAGS_NONE"
    additional_bytes : int = 0
    handler : str = "nullptr"

class X86TableEntry(NamedTuple):
    opcode : int
    count : int
    Info : X86InstInfo

def MakeTuple(Op, Size, Tuple):
    return X86TableEntry(Op, Size, X86InstInfo(*Tuple))

# TODO: NOP
class base_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

class ddd_table:
    DescriptionTable = [
        (0x0C, 1, ["PI2FW",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x0D, 1, ["PI2FD",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x1C, 1, ["PF2IW",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x1D, 1, ["PF2ID",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        # Inverse 3DNow! These two instructions are Geode product line specific
        # No CPUID for these, you're expected to read ID_CONFIG_MSR (1250h) bit 1
        (0x86, 1, ["PFRCPV",   InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x87, 1, ["PFRSQRTV", InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0x8A, 1, ["PFNACC",   InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x8E, 1, ["PFPNACC",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0x90, 1, ["PFCMPGE",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x94, 1, ["PFMIN",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x96, 1, ["PFRCP",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x97, 1, ["PFRSQRT",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0x9A, 1, ["PFSUB",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0x9E, 1, ["PFADD",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0xA0, 1, ["PFCMPGT",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xA4, 1, ["PFMAX",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xA6, 1, ["PFRCPIT1", InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xA7, 1, ["PFRSQIT1", InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0xAA, 1, ["PFSUBR",   InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xAE, 1, ["PFACC",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0xB0, 1, ["PFCMPEQ",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xB4, 1, ["PFMUL",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xB6, 1, ["PFRCPIT2", InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xB7, 1, ["PMULHRW",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),

        (0xBB, 1, ["PSWAPD",   InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (0xBF, 1, ["PAVGUSB",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
    ]

    DispatchTable = [
        [0x0C, 1, "&IR::OpDispatchBuilder::PI2FWOp"],
        [0x0D, 1, "&IR::OpDispatchBuilder::Vector_CVT_Int_To_Float<IR::OpSize::i32Bit, false>"],
        [0x1C, 1, "&IR::OpDispatchBuilder::PF2IWOp"],
        [0x1D, 1, "&IR::OpDispatchBuilder::Vector_CVT_Float_To_Int<IR::OpSize::i32Bit, false>"],

        [0x86, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VFRECPPRECISION, IR::OpSize::i32Bit>"],
        [0x87, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::RSqrt3DNowOp, false>"],

        [0x8A, 1, "&IR::OpDispatchBuilder::PFNACCOp"],
        [0x8E, 1, "&IR::OpDispatchBuilder::PFPNACCOp"],

        [0x90, 1, "&IR::OpDispatchBuilder::VPFCMPOp<1>"],
        [0x94, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VFMIN, IR::OpSize::i32Bit>"],
        [0x96, 1, "&IR::OpDispatchBuilder::VectorUnaryDuplicateOp<IR::OP_VFRECPPRECISION, IR::OpSize::i32Bit>"],
        [0x97, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::RSqrt3DNowOp, true>"],

        [0x9A, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VFSUB, IR::OpSize::i32Bit>"],
        [0x9E, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VFADD, IR::OpSize::i32Bit>"],

        [0xA0, 1, "&IR::OpDispatchBuilder::VPFCMPOp<2>"],
        [0xA4, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VFMAX, IR::OpSize::i32Bit>"],
        # Can be treated as a move
        [0xA6, 1, "&IR::OpDispatchBuilder::MOVVectorUnalignedOp"],
        [0xA7, 1, "&IR::OpDispatchBuilder::MOVVectorUnalignedOp"],

        [0xAA, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUROp, IR::OP_VFSUB, IR::OpSize::i32Bit>"],
        [0xAE, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VFADDP, IR::OpSize::i32Bit>"],

        [0xB0, 1, "&IR::OpDispatchBuilder::VPFCMPOp<0>"],
        [0xB4, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VFMUL, IR::OpSize::i32Bit>"],
        # Can be treated as a move
        [0xB6, 1, "&IR::OpDispatchBuilder::MOVVectorUnalignedOp"],
        [0xB7, 1, "&IR::OpDispatchBuilder::PMULHRWOp"],

        [0xBB, 1, "&IR::OpDispatchBuilder::PSWAPDOp"],
        [0xBF, 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VURAVG, IR::OpSize::i8Bit>"],
    ]

class h0f38_table:
    PF_38_NONE = 0
    PF_38_66 = 1 << 0
    PF_38_F2 = 1 << 1
    PF_38_F3 = 1 << 2

    def OPD(Prefix, Base):
        return Prefix << 8 | Base

    DescriptionTable = [
        (OPD(PF_38_NONE, 0x00), 1, [ "PSHUFB",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x00), 1, [ "PSHUFB",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x01), 1, [ "PHADDW",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x01), 1, [ "PHADDW",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x02), 1, [ "PHADDD",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x02), 1, [ "PHADDD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x03), 1, [ "PHADDSW",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x03), 1, [ "PHADDSW",    InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x04), 1, [ "PMADDUBSW",  InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x04), 1, [ "PMADDUBSW",  InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x05), 1, [ "PHSUBW",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x05), 1, [ "PHSUBW",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x06), 1, [ "PHSUBD",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x06), 1, [ "PHSUBD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x07), 1, [ "PHSUBSW",    InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x07), 1, [ "PHSUBSW",    InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x08), 1, [ "PSIGNB",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x08), 1, [ "PSIGNB",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x09), 1, [ "PSIGNW",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x09), 1, [ "PSIGNW",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x0A), 1, [ "PSIGND",     InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x0A), 1, [ "PSIGND",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x0B), 1, [ "PMULHRSW",   InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x0B), 1, [ "PMULHRSW",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_66,   0x10), 1, [ "PBLENDVB",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x14), 1, [ "BLENDVPS",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x15), 1, [ "BLENDVPD",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x17), 1, [ "PTEST",      InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x1C), 1, [ "PABSB",      InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x1C), 1, [ "PABSB",      InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x1D), 1, [ "PABSW",      InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x1D), 1, [ "PABSW",      InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0x1E), 1, [ "PABSD",      InstType.INST, "GenFlagsSameSize(SIZE_64BIT)  | FLAGS_MODRM | FLAGS_XMM_FLAGS | FLAGS_SF_MMX"]),
        (OPD(PF_38_66,   0x1E), 1, [ "PABSD",      InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_66,   0x20), 1, [ "PMOVSXBW",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x21), 1, [ "PMOVSXBD",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x22), 1, [ "PMOVSXBQ",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x23), 1, [ "PMOVSXWD",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x24), 1, [ "PMOVSXWQ",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x25), 1, [ "PMOVSXDQ",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x28), 1, [ "PMULDQ",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x29), 1, [ "PCMPEQQ",    InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x2A), 1, [ "MOVNTDQA",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x2B), 1, [ "PACKUSDW",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_66,   0x30), 1, [ "PMOVZXBW",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x31), 1, [ "PMOVZXBD",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x32), 1, [ "PMOVZXBQ",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_16BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x33), 1, [ "PMOVZXWD",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x34), 1, [ "PMOVZXWQ",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_32BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x35), 1, [ "PMOVZXDQ",   InstType.INST, "GenFlagsSizes(SIZE_128BIT, SIZE_64BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x37), 1, [ "PCMPGTQ",    InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x38), 1, [ "PMINSB",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x39), 1, [ "PMINSD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x3A), 1, [ "PMINUW",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x3B), 1, [ "PMINUD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x3C), 1, [ "PMAXSB",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x3D), 1, [ "PMAXSD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x3E), 1, [ "PMAXUW",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x3F), 1, [ "PMAXUD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_66,   0x40), 1, [ "PMULLD",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0x41), 1, [ "PHMINPOSUW", InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_NONE, 0xC8), 1, [ "SHA1NEXTE",  InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0xC9), 1, [ "SHA1MSG1",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0xCA), 1, [ "SHA1MSG2",   InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_NONE, 0xCB), 1, [ "SHA256RNDS2", InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0xCC), 1, [ "SHA256MSG1",  InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_NONE, 0xCD), 1, [ "SHA256MSG2",  InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_66,   0xDB), 1, [ "AESIMC",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0xDC), 1, [ "AESENC",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0xDD), 1, [ "AESENCLAST", InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0xDE), 1, [ "AESDEC",     InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),
        (OPD(PF_38_66,   0xDF), 1, [ "AESDECLAST", InstType.INST, "GenFlagsSameSize(SIZE_128BIT) | FLAGS_MODRM | FLAGS_XMM_FLAGS"]),

        (OPD(PF_38_NONE, 0xF0), 1, [ "MOVBE",      InstType.INST, "FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY"]),
        (OPD(PF_38_NONE, 0xF1), 1, [ "MOVBE",      InstType.INST, "FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY"]),

        (OPD(PF_38_66, 0xF0), 1, [ "MOVBE",      InstType.INST, "FLAGS_MODRM | FLAGS_SF_MOD_MEM_ONLY"]),
        (OPD(PF_38_66, 0xF1), 1, [ "MOVBE",      InstType.INST, "FLAGS_MODRM | FLAGS_SF_MOD_DST | FLAGS_SF_MOD_MEM_ONLY"]),

        (OPD(PF_38_F2,   0xF0), 1, [ "CRC32",      InstType.INST, "GenFlagsSizes(SIZE_DEF, SIZE_8BIT) | FLAGS_MODRM"]),
        (OPD(PF_38_F2,   0xF1), 1, [ "CRC32",      InstType.INST, "FLAGS_MODRM"]),
        (OPD(PF_38_66 | PF_38_F2,   0xF0), 1, [ "CRC32",      InstType.INST, "GenFlagsSizes(SIZE_DEF, SIZE_8BIT) | FLAGS_MODRM"]),
        (OPD(PF_38_66 | PF_38_F2,   0xF1), 1, [ "CRC32",      InstType.INST, "FLAGS_MODRM"]),

        (OPD(PF_38_66,   0xF6), 1, [ "ADCX",       InstType.INST, "FLAGS_MODRM | FLAGS_NO_OVERLAY66"]),
        (OPD(PF_38_F3,   0xF6), 1, [ "ADOX",       InstType.INST, "FLAGS_MODRM"]),
    ]

    DispatchTable = [
        [OPD(PF_38_NONE, 0x00), 1, "&IR::OpDispatchBuilder::PSHUFBOp"],
        [OPD(PF_38_66, 0x00), 1, "&IR::OpDispatchBuilder::PSHUFBOp"],
        [OPD(PF_38_NONE, 0x01), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, IR::OpSize::i16Bit>"],
        [OPD(PF_38_66, 0x01), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, IR::OpSize::i16Bit>"],
        [OPD(PF_38_NONE, 0x02), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x02), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VADDP, IR::OpSize::i32Bit>"],
        [OPD(PF_38_NONE, 0x03), 1, "&IR::OpDispatchBuilder::PHADDS"],
        [OPD(PF_38_66, 0x03), 1, "&IR::OpDispatchBuilder::PHADDS"],
        [OPD(PF_38_NONE, 0x04), 1, "&IR::OpDispatchBuilder::PMADDUBSW"],
        [OPD(PF_38_66, 0x04), 1, "&IR::OpDispatchBuilder::PMADDUBSW"],
        [OPD(PF_38_NONE, 0x05), 1, "&IR::OpDispatchBuilder::PHSUB<IR::OpSize::i16Bit>"],
        [OPD(PF_38_66, 0x05), 1, "&IR::OpDispatchBuilder::PHSUB<IR::OpSize::i16Bit>"],
        [OPD(PF_38_NONE, 0x06), 1, "&IR::OpDispatchBuilder::PHSUB<IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x06), 1, "&IR::OpDispatchBuilder::PHSUB<IR::OpSize::i32Bit>"],
        [OPD(PF_38_NONE, 0x07), 1, "&IR::OpDispatchBuilder::PHSUBS"],
        [OPD(PF_38_66, 0x07), 1, "&IR::OpDispatchBuilder::PHSUBS"],
        [OPD(PF_38_NONE, 0x08), 1, "&IR::OpDispatchBuilder::PSIGN<IR::OpSize::i8Bit>"],
        [OPD(PF_38_66, 0x08), 1, "&IR::OpDispatchBuilder::PSIGN<IR::OpSize::i8Bit>"],
        [OPD(PF_38_NONE, 0x09), 1, "&IR::OpDispatchBuilder::PSIGN<IR::OpSize::i16Bit>"],
        [OPD(PF_38_66, 0x09), 1, "&IR::OpDispatchBuilder::PSIGN<IR::OpSize::i16Bit>"],
        [OPD(PF_38_NONE, 0x0A), 1, "&IR::OpDispatchBuilder::PSIGN<IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x0A), 1, "&IR::OpDispatchBuilder::PSIGN<IR::OpSize::i32Bit>"],
        [OPD(PF_38_NONE, 0x0B), 1, "&IR::OpDispatchBuilder::PMULHRSW"],
        [OPD(PF_38_66, 0x0B), 1, "&IR::OpDispatchBuilder::PMULHRSW"],
        [OPD(PF_38_66, 0x10), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorVariableBlend, IR::OpSize::i8Bit>"],
        [OPD(PF_38_66, 0x14), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorVariableBlend, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x15), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorVariableBlend, IR::OpSize::i64Bit>"],
        [OPD(PF_38_66, 0x17), 1, "&IR::OpDispatchBuilder::PTestOp"],
        [OPD(PF_38_NONE, 0x1C), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, IR::OpSize::i8Bit>"],
        [OPD(PF_38_66, 0x1C), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, IR::OpSize::i8Bit>"],
        [OPD(PF_38_NONE, 0x1D), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, IR::OpSize::i16Bit>"],
        [OPD(PF_38_66, 0x1D), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, IR::OpSize::i16Bit>"],
        [OPD(PF_38_NONE, 0x1E), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x1E), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorUnaryOp, IR::OP_VABS, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x20), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i8Bit, IR::OpSize::i16Bit, true>"],
        [OPD(PF_38_66, 0x21), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i8Bit, IR::OpSize::i32Bit, true>"],
        [OPD(PF_38_66, 0x22), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i8Bit, IR::OpSize::i64Bit, true>"],
        [OPD(PF_38_66, 0x23), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i16Bit, IR::OpSize::i32Bit, true>"],
        [OPD(PF_38_66, 0x24), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i16Bit, IR::OpSize::i64Bit, true>"],
        [OPD(PF_38_66, 0x25), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i32Bit, IR::OpSize::i64Bit, true>"],
        [OPD(PF_38_66, 0x28), 1, "&IR::OpDispatchBuilder::PMULLOp<IR::OpSize::i32Bit, true>"],
        [OPD(PF_38_66, 0x29), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VCMPEQ, IR::OpSize::i64Bit>"],
        [OPD(PF_38_66, 0x2A), 1, "&IR::OpDispatchBuilder::MOVVectorNTOp"],
        [OPD(PF_38_66, 0x2B), 1, "&IR::OpDispatchBuilder::PACKUSOp<IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x30), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i8Bit, IR::OpSize::i16Bit, false>"],
        [OPD(PF_38_66, 0x31), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i8Bit, IR::OpSize::i32Bit, false>"],
        [OPD(PF_38_66, 0x32), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i8Bit, IR::OpSize::i64Bit, false>"],
        [OPD(PF_38_66, 0x33), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i16Bit, IR::OpSize::i32Bit, false>"],
        [OPD(PF_38_66, 0x34), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i16Bit, IR::OpSize::i64Bit, false>"],
        [OPD(PF_38_66, 0x35), 1, "&IR::OpDispatchBuilder::ExtendVectorElements<IR::OpSize::i32Bit, IR::OpSize::i64Bit, false>"],
        [OPD(PF_38_66, 0x37), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VCMPGT, IR::OpSize::i64Bit>"],
        [OPD(PF_38_66, 0x38), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VSMIN, IR::OpSize::i8Bit>"],
        [OPD(PF_38_66, 0x39), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VSMIN, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x3A), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VUMIN, IR::OpSize::i16Bit>"],
        [OPD(PF_38_66, 0x3B), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VUMIN, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x3C), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VSMAX, IR::OpSize::i8Bit>"],
        [OPD(PF_38_66, 0x3D), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VSMAX, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x3E), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VUMAX, IR::OpSize::i16Bit>"],
        [OPD(PF_38_66, 0x3F), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VUMAX, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x40), 1, "&IR::OpDispatchBuilder::Bind<&IR::OpDispatchBuilder::VectorALUOp, IR::OP_VMUL, IR::OpSize::i32Bit>"],
        [OPD(PF_38_66, 0x41), 1, "&IR::OpDispatchBuilder::PHMINPOSUWOp"],

        [OPD(PF_38_NONE, 0xF0), 2, "&IR::OpDispatchBuilder::MOVBEOp"],
        [OPD(PF_38_66, 0xF0), 2, "&IR::OpDispatchBuilder::MOVBEOp"],

        [OPD(PF_38_66, 0xF6), 1, "&IR::OpDispatchBuilder::ADXOp"],
        [OPD(PF_38_F3, 0xF6), 1, "&IR::OpDispatchBuilder::ADXOp"],

        # Describe SHA, AES, CRC to install the configurations in the table. Might be disabled at runtime.
        # SHA
        [OPD(PF_38_NONE, 0xC8), 1, "&IR::OpDispatchBuilder::SHA1NEXTEOp"],
        [OPD(PF_38_NONE, 0xC9), 1, "&IR::OpDispatchBuilder::SHA1MSG1Op"],
        [OPD(PF_38_NONE, 0xCA), 1, "&IR::OpDispatchBuilder::SHA1MSG2Op"],
        [OPD(PF_38_NONE, 0xCB), 1, "&IR::OpDispatchBuilder::SHA256RNDS2Op"],
        [OPD(PF_38_NONE, 0xCC), 1, "&IR::OpDispatchBuilder::SHA256MSG1Op"],
        [OPD(PF_38_NONE, 0xCD), 1, "&IR::OpDispatchBuilder::SHA256MSG2Op"],

        # AES
        [OPD(PF_38_66, 0xDB), 1, "&IR::OpDispatchBuilder::AESImcOp"],
        [OPD(PF_38_66, 0xDC), 1, "&IR::OpDispatchBuilder::AESEncOp"],
        [OPD(PF_38_66, 0xDD), 1, "&IR::OpDispatchBuilder::AESEncLastOp"],
        [OPD(PF_38_66, 0xDE), 1, "&IR::OpDispatchBuilder::AESDecOp"],
        [OPD(PF_38_66, 0xDF), 1, "&IR::OpDispatchBuilder::AESDecLastOp"],

        # CRC
        [OPD(PF_38_F2, 0xF0), 1, "&IR::OpDispatchBuilder::CRC32"],
        [OPD(PF_38_F2, 0xF1), 1, "&IR::OpDispatchBuilder::CRC32"],

        [OPD(PF_38_66 | PF_38_F2, 0xF0), 1, "&IR::OpDispatchBuilder::CRC32"],
        [OPD(PF_38_66 | PF_38_F2, 0xF1), 1, "&IR::OpDispatchBuilder::CRC32"],
    ]

# TODO: NOP
class h0f3a_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# TODO: NOP
class primary_group_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# TODO: NOP
class secondary_group_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# TODO: NOP
class secondary_modrm_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# TODO: NOP
class secondary_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# TODO: NOP
class vex_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# TODO: NOP
class x87_table:
    DescriptionTable = [
    ]

    DispatchTable = [
    ]

# Just bifurcates table opcodes by their counts.
def generate_base_table(table, max_size):
    table_list = [None] * max_size
    for op in table:
        op = MakeTuple(*op)
        for i in range(op.count):
            table_list[op.opcode + i] = op.Info

    return table_list

# merge a dispatch table in to an opcode table
def merge_dispatch_tables(table, dispatch_table):
    table_list = [None] * len(table)

    for dispatch_op in dispatch_table:
        opcode = dispatch_op[0]
        count = dispatch_op[1]
        handler = dispatch_op[2]

        for i in range(count):
            info = table[opcode + i]
            assert info != None
            assert info.name != None
            assert info.handler == "nullptr"

            table_list[opcode + i] = X86InstInfo(name = info.name, type = info.type, flags = info.flags, additional_bytes = info.additional_bytes,
                                                 handler = handler)

    return table_list

def print_table(folder, filename, name, table):
    output = open("{}/{}".format(folder, filename), "w")

    output.write("constexpr std::array<X86InstInfo, {}> {} = {{{{\n".format(len(table), name))

    UndefinedOp = "X86InstInfo{nullptr, TYPE_INVALID, FLAGS_NONE, 0, {.OpDispatch = nullptr}},"
    for op in table:
        if op == None:
            output.write("  {}\n".format(UndefinedOp))
            continue

        output.write("  {{\"{}\", TYPE_{}, {}, {}, {{ .OpDispatch = {} }} }},\n".format(op.name, op.type.name, op.flags, op.additional_bytes, op.handler))

    output.write("}};");

    output.close()


if (len(sys.argv) < 2):
    sys.exit()

output_folder = sys.argv[1]

# DDD Table
base_table = generate_base_table(ddd_table.DescriptionTable, 256)
merged_table = merge_dispatch_tables(base_table, ddd_table.DispatchTable)
print_table(output_folder, "DDDTables.inl", "DDDNowOps", merged_table)

# H0F38 Table
base_table = generate_base_table(h0f38_table.DescriptionTable, 1 << 11)
merged_table = merge_dispatch_tables(base_table, h0f38_table.DispatchTable)
print_table(output_folder, "H0F38Tables.inl", "H0F38TableOps", merged_table)
