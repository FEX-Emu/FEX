%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4142434445464748", "0x6162636465666768"],
    "XMM1": ["0x6162636465666768", "0x7172737475767778"]
  }
}
%endif

lea rdx, [rel .data]

movaps xmm0, [rdx]
movaps xmm1, [rdx + 16]

movlhps xmm0, xmm1

hlt

align 16
.data:
dq 0x4142434445464748
dq 0x5152535455565758

dq 0x6162636465666768
dq 0x7172737475767778
