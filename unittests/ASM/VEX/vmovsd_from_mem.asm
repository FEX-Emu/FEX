%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x4142434445464748", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx + 32]

; Move data into register
vmovsd xmm0, [rdx]

hlt

align 32
.data:
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x0000000000000000
dq 0x0000000000000000

dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
dq 0xFFFFFFFFFFFFFFFF
