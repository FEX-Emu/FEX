%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM1": [
      "0xd89f3a05a4322f49", 
      "0x7fd09a85846a3fbb", 
      "0xc5e072b814de6cb0", 
      "0xb7050315bf62d810"
    ],
    "XMM2": [
      "0x7e848001fc008001", 
      "0", 
      "0", 
      "0"
    ],
    "XMM3": [
      "0x7e848001fc008001", 
      "0x8086bb17ef040000", 
      "0", 
      "0"
    ],
    "XMM4": [
      "0x7e848001fc008001", 
      "0x8086bb17ef040000", 
      "0x3333333333333333", 
      "0x4444444444444444"
    ]
  }
}
%endif

lea rdx, [rel .data]

vmovdqa ymm1, [rdx]
vmovdqa ymm2, [rdx + 32]
vmovdqa ymm3, [rdx + 32]

vcvtps2ph xmm2, xmm1, 0x1

vcvtps2ph xmm3, ymm1, 0x1

vcvtps2ph [rdx + 64], ymm1, 0x1
vmovdqa ymm4, [rdx + 64]

hlt

align 4096
.data:
; ymm1
dq 0xd89f3a05a4322f49, 0x7fd09a85846a3fbb, 0xc5e072b814de6cb0, 0xb7050315bf62d810
; ymm2
dq 0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF
; ymm4 memory
dq 0x1111111111111111, 0x2222222222222222, 0x3333333333333333, 0x4444444444444444