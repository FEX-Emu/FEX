%ifdef CONFIG
{
  "RegData": {
      "XMM1": ["0x1E2017C5BEE29400", "0x38358E40CC367C7A"],
      "XMM2": ["0x6868C3F3AAED56E0", "0xF0FCE9E294E6E6DE"],
      "XMM3": ["0xE208147952DE57A0", "0x317D360F86C80DC9"],
      "XMM4": ["0xBBA54C87DA872B40", "0x6495428B7641EBE6"],
      "XMM5": ["0x170B5A1B5CDD42EA", "0x719F094BB2358CA1"]
  },
  "HostFeatures": ["PCLMUL"]
}
%endif

lea rdx, [rel .data]

; With imm = 0b00000000
movaps xmm1, [rdx + 16 * 0]
movaps xmm2, [rdx + 16 * 1]
pclmulqdq xmm1, xmm2, 0

; With imm = 0b00000001
movaps xmm3, [rdx + 16 * 0]
pclmulqdq xmm3, xmm2, 1

; With imm = 0b00010000
movaps xmm4, [rdx + 16 * 0]
pclmulqdq xmm4, xmm2, 16

; With imm = 0b00010001
movaps xmm5, [rdx + 16 * 0]
pclmulqdq xmm5, xmm2, 17

hlt

align 16
.data:
db 0xe0, 0xfc, 0x2b, 0xa1, 0x06, 0x4f, 0x6c, 0xa7, 0x0f, 0x06, 0x6a, 0x1e, 0x7f, 0x76, 0x80, 0x9b
db 0xe0, 0x56, 0xed, 0xaa, 0xf3, 0xc3, 0x68, 0x68, 0xde, 0xe6, 0xe6, 0x94, 0xe2, 0xe9, 0xfc, 0xf0
