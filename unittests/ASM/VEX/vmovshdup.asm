%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM1": ["0xEEEEEEEEEEEEEEEE", "0xCCCCCCCCCCCCCCCC", "0xAAAAAAAAAAAAAAAA", "0x0808080808080808"],
      "XMM2": ["0xEEEEEEEEEEEEEEEE", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"],
      "XMM3": ["0xEEEEEEEEFFFFFFFF", "0xCCCCCCCCDDDDDDDD", "0xAAAAAAAABBBBBBBB", "0x0808080809090909"],
      "XMM4": ["0xEEEEEEEEEEEEEEEE", "0xCCCCCCCCCCCCCCCC", "0xAAAAAAAAAAAAAAAA", "0x0808080808080808"],
      "XMM5": ["0xEEEEEEEEEEEEEEEE", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"],
      "XMM6": ["0xEEEEEEEEEEEEEEEE", "0xCCCCCCCCCCCCCCCC", "0xAAAAAAAAAAAAAAAA", "0x0808080808080808"],
      "XMM7": ["0xEEEEEEEEEEEEEEEE", "0xCCCCCCCCCCCCCCCC", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

;; Broadcast across self
vmovaps ymm1, [rdx]
vmovshdup ymm1, ymm1
; 128-bit version
vmovaps xmm2, [rdx]
vmovshdup xmm2, xmm2

;; Broadcast from different registers
vmovaps ymm3, [rdx]
vmovshdup ymm4, ymm3
; 128-bit version
vmovshdup xmm5, xmm3

;; Broadcast from memory
vmovshdup ymm6, [rdx]
; 128-bit version
vmovshdup xmm7, [rdx]

hlt

align 32
.data:
db 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xEE, 0xEE, 0xEE, 0xDD, 0xDD, 0xDD, 0xDD, 0xCC, 0xCC, 0xCC, 0xCC
db 0xBB, 0xBB, 0xBB, 0xBB, 0xAA, 0xAA, 0xAA, 0xAA, 0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08
