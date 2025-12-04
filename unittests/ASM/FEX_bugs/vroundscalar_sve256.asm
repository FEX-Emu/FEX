%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x818283843f800000", "0x9192939495969798", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

; FEX-Emu had a bug in its 256-bit SVE implementation of AVX where scalar round with insert wasn't inserting correctly.
; This tests to ensure that the sources are merged correctly.
vmovaps ymm0, [rel .data_trash]
vmovaps ymm1, [rel .data_trash + 32]

vroundss xmm0, xmm1, [rel .data], 00000010b ; +inf

hlt

align 4096
.data:
dd 0.5, -0.5, 1.5, -1.5
dd 0.5, -0.5, 1.5, -1.5

.data_trash:
dq 0x4142434445464748, 0x5152535455565758
dq 0x6162636465666768, 0x7172737475767778
dq 0x8182838485868788, 0x9192939495969798
dq 0xA1A2A3A4A5A6A7A8, 0xB1B2B3B4B5B6B7B8
