%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM1": ["0xA76C4F06A12BFCE0", "0x9B80767F1E6A060F", "0xFFFFFFFFFFFFFFFF", "0xEEEEEEEEEEEEEEEE"],
      "XMM2": ["0x6868C3F3AAED56E0", "0xF0FCE9E294E6E6DE", "0xDDDDDDDDDDDDDDDD", "0xCCCCCCCCCCCCCCCC"],
      "XMM3": ["0x1E2017C5BEE29400", "0x38358E40CC367C7A", "0x0000000000000000", "0x0000000000000000"],
      "XMM4": ["0xE208147952DE57A0", "0x317D360F86C80DC9", "0x0000000000000000", "0x0000000000000000"],
      "XMM5": ["0xBBA54C87DA872B40", "0x6495428B7641EBE6", "0x0000000000000000", "0x0000000000000000"],
      "XMM6": ["0x170B5A1B5CDD42EA", "0x719F094BB2358CA1", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

; Load inputs
vmovaps ymm1, [rdx + 32 * 0]
vmovaps ymm2, [rdx + 32 * 1]

; Fill result vectors with junk (ensure proper lane clearing is performed)
vmovaps ymm3, [rdx + 32 * 0]
vmovaps ymm4, [rdx + 32 * 0]
vmovaps ymm5, [rdx + 32 * 0]
vmovaps ymm6, [rdx + 32 * 0]

; With imm = 0b00000000
vpclmulqdq xmm3, xmm1, xmm2, 0

; With imm = 0b00000001
vpclmulqdq xmm4, xmm1, xmm2, 1

; With imm = 0b00010000
vpclmulqdq xmm5, xmm1, xmm2, 16

; With imm = 0b00010001
vpclmulqdq xmm6, xmm1, xmm2, 17

hlt

align 32
.data:
dq 0xA76C4F06A12BFCE0
dq 0x9B80767F1E6A060F
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE
dq 0x6868C3F3AAED56E0
dq 0xF0FCE9E294E6E6DE
dq 0xDDDDDDDDDDDDDDDD
dq 0xCCCCCCCCCCCCCCCC
