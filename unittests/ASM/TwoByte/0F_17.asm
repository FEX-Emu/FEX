%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4142434445464748", "0x6162636465666768"],
    "XMM1": ["0x6162636465666768", "0xFFFFFFFFFFFFFFFF"]
  }
}
%endif

lea rdx, [rel .data]

; Into register
movaps xmm0, [rdx]
movhps xmm0, [rdx + 16]

; Into memory (should only store upper half of xmm into 64-bit region of memory)
movhps [rdx + 32], xmm0
movaps xmm1, [rdx + 32]

hlt

align 4096
.data:
dq 0x4142434445464748
dq 0x5152535455565758

dq 0x6162636465666768
dq 0x7172737475767778

dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
