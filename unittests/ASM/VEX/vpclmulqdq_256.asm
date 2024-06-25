%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM1": ["0xA76C4F06A12BFCE0", "0x9B80767F1E6A060F", "0xFFFFFFFFFFFFFFFF", "0xEEEEEEEEEEEEEEEE"],
      "XMM2": ["0x6868C3F3AAED56E0", "0xF0FCE9E294E6E6DE", "0xDDDDDDDDDDDDDDDD", "0xCCCCCCCCCCCCCCCC"],
      "XMM3": ["0x1E2017C5BEE29400", "0x38358E40CC367C7A", "0x4b4b4b4b4b4b4b4b", "0x4b4b4b4b4b4b4b4b"],
      "XMM4": ["0xE208147952DE57A0", "0x317D360F86C80DC9", "0x4646464646464646", "0x4646464646464646"],
      "XMM5": ["0xBBA54C87DA872B40", "0x6495428B7641EBE6", "0x4444444444444444", "0x4444444444444444"],
      "XMM6": ["0x170B5A1B5CDD42EA", "0x719F094BB2358CA1", "0x4848484848484848", "0x4848484848484848"],
      "XMM7": ["0x1e2017c5bee29400", "0x38358e40cc367c7a", "0", "0"]
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
vmovaps ymm7, [rdx + 32 * 0]

; With imm = 0b00000000
vpclmulqdq ymm3, ymm1, ymm2, 0

; With imm = 0b00000001
vpclmulqdq ymm4, ymm1, ymm2, 1

; With imm = 0b00010000
vpclmulqdq ymm5, ymm1, ymm2, 16

; With imm = 0b00010001
vpclmulqdq ymm6, ymm1, ymm2, 17

; Test zero-extension
; Also test a wacky immediate.
vpclmulqdq xmm7, xmm1, xmm2, 11101110b

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
