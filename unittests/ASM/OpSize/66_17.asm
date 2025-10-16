%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x4142434445464748", "0x5152535455565758"],
    "XMM1": ["0x5152535455565758", "0xFFFFFFFFFFFFFFFF"]
  }
}
%endif

lea rdx, [rel .data]

movapd xmm0, [rdx]
movhpd [rdx + 16], xmm0
movapd xmm1, [rdx + 16]

hlt

align 4096
.data:
dq 0x4142434445464748
dq 0x5152535455565758

dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
