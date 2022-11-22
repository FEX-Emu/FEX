%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM1": ["0xFFFFFFFFFFFFFFFF", "0xDDDDDDDDDDDDDDDD", "0xBBBBBBBBBBBBBBBB", "0x0909090909090909"],
      "XMM2": ["0xFFFFFFFFFFFFFFFF", "0xDDDDDDDDDDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
      "XMM3": ["0xEEEEEEEEFFFFFFFF", "0xCCCCCCCCDDDDDDDD", "0xAAAAAAAABBBBBBBB", "0x0808080809090909"],
      "XMM4": ["0xFFFFFFFFFFFFFFFF", "0xDDDDDDDDDDDDDDDD", "0xBBBBBBBBBBBBBBBB", "0x0909090909090909"],
      "XMM5": ["0xFFFFFFFFFFFFFFFF", "0xDDDDDDDDDDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
      "XMM6": ["0xFFFFFFFFFFFFFFFF", "0xDDDDDDDDDDDDDDDD", "0xBBBBBBBBBBBBBBBB", "0x0909090909090909"],
      "XMM7": ["0xFFFFFFFFFFFFFFFF", "0xDDDDDDDDDDDDDDDD", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

;; Broadcast across self
vmovaps ymm1, [rdx]
vmovsldup ymm1, ymm1
; 128-bit version
vmovaps xmm2, [rdx]
vmovsldup xmm2, xmm2

;; Broadcast from different registers
vmovaps ymm3, [rdx]
vmovsldup ymm4, ymm3
; 128-bit version
vmovsldup xmm5, xmm3

;; Broadcast from memory
vmovsldup ymm6, [rdx]
; 128-bit version
vmovsldup xmm7, [rdx]

hlt

align 32
.data:
db 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xEE, 0xEE, 0xEE, 0xDD, 0xDD, 0xDD, 0xDD, 0xCC, 0xCC, 0xCC, 0xCC
db 0xBB, 0xBB, 0xBB, 0xBB, 0xAA, 0xAA, 0xAA, 0xAA, 0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08
