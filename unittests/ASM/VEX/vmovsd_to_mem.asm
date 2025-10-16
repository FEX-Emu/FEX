%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x4142434445464748", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd xmm0, [rdx]

; Moves lower 64bits to memory
vmovsd [rdx + 16], xmm0

; Ensure 128bits weren't written
vmovapd xmm0, [rdx + 16]

hlt

align 4096
.data:
dq 0x4142434445464748
dq 0x5152535455565758
dq 0x0000000000000000
dq 0x0000000000000000
