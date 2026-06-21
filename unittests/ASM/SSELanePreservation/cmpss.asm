%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x1c80317fffffffff", "0xbdd640fb06671ad1", "0x3eb13b9046685257", "0x23b8c1e9392456de"],
    "XMM1":  ["0x1c80317fffffffff", "0xbdd640fb06671ad1", "0x0000000000000000", "0x0000000000000000"],
    "XMM2":  ["0x1a3d1fa700000000", "0xbd9c66b3ad3c2d6d", "0x8b9d2434e465e150", "0x972a846916419f82"],
    "XMM3":  ["0x1a3d1fa700000000", "0xbd9c66b3ad3c2d6d", "0x0000000000000000", "0x0000000000000000"],
    "XMM4":  ["0x0822e8f3ffffffff", "0x17fc695a07a0ca6e", "0x3b8faa1837f8a88b", "0x9a1de644815ef6d1"],
    "XMM5":  ["0x0822e8f3ffffffff", "0x17fc695a07a0ca6e", "0x0000000000000000", "0x0000000000000000"],
    "XMM6":  ["0x8fadc1a6ffffffff", "0xb74d0fb132e70629", "0xb38a088ca65ed389", "0x6b65a6a48b8148f6"],
    "XMM7":  ["0x8fadc1a6ffffffff", "0xb74d0fb132e70629", "0x0000000000000000", "0x0000000000000000"],
    "XMM8":  ["0x72ff5d2affffffff", "0x4737819096da1dac", "0xde8a774bcf36d58b", "0xc241330b01a9e71f"],
    "XMM9":  ["0x72ff5d2affffffff", "0x4737819096da1dac", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0x28df6ec400000000", "0x6c307511b2b9437a", "0x47229389571aa876", "0x371ecd7b27cd8130"],
    "XMM11": ["0x28df6ec400000000", "0x6c307511b2b9437a", "0x0000000000000000", "0x0000000000000000"],
    "XMM12": ["0xc37459eeffffffff", "0x1a2a73ed562b0f79", "0x6142ea7d17be3111", "0x5be6128e18c26797"],
    "XMM13": ["0xc37459eeffffffff", "0x1a2a73ed562b0f79", "0x0000000000000000", "0x0000000000000000"],
    "XMM14": ["0x580d7b7100000000", "0x43b7a3a69a8dca03", "0x0b1f9163ce9ff57f", "0x759cde66bacfb3d0"],
    "XMM15": ["0x580d7b7100000000", "0x43b7a3a69a8dca03", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

; Test to ensure every path of cmpsd is covered and handles lane
; preservation properly.

%include "sse_scalar_compare_utils.mac"

load_all_initial_data
perform_compare_tests cmpss, 0

hlt

; Initial data to put into registers before the test.
; Consists of a valid float geared for the particular test followed by junk data for the upper lanes.
align 32
.data:
dq 0x1c80317f3f800000, 0xbdd640fb06671ad1, 0x3eb13b9046685257, 0x23b8c1e9392456de
dq 0x1a3d1fa740000000, 0xbd9c66b3ad3c2d6d, 0x8b9d2434e465e150, 0x972a846916419f82
dq 0x0822e8f33f800000, 0x17fc695a07a0ca6e, 0x3b8faa1837f8a88b, 0x9a1de644815ef6d1
dq 0x8fadc1a63f800000, 0xb74d0fb132e70629, 0xb38a088ca65ed389, 0x6b65a6a48b8148f6
dq 0x72ff5d2a3f800000, 0x4737819096da1dac, 0xde8a774bcf36d58b, 0xc241330b01a9e71f
dq 0x28df6ec43f800000, 0x6c307511b2b9437a, 0x47229389571aa876, 0x371ecd7b27cd8130
dq 0xc37459ee3f800000, 0x1a2a73ed562b0f79, 0x6142ea7d17be3111, 0x5be6128e18c26797
dq 0x580d7b713f800000, 0x43b7a3a69a8dca03, 0x0b1f9163ce9ff57f, 0x759cde66bacfb3d0

; What we use as the source memory operand for a particular test.
align 32
.scalar_data:
dd 0x3f800000 ; 1.0
dd 0x3f800000 ; 1.0
dd 0x40000000 ; 2.0
dd 0x7fc00000 ; NaN (positive)
dd 0x00000000 ; 0.0
dd 0x40000000 ; 2.0
dd 0x00000000 ; 0.0
dd 0xffc00000 ; NaN (negative)
