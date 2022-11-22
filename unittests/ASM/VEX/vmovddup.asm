%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM0": ["0xEEEEEEEEFFFFFFFF", "0xCCCCCCCCDDDDDDDD", "0xAAAAAAAABBBBBBBB", "0x0808080809090909"],
      "XMM1": ["0xEEEEEEEEFFFFFFFF", "0xEEEEEEEEFFFFFFFF", "0xAAAAAAAABBBBBBBB", "0xAAAAAAAABBBBBBBB"],
      "XMM2": ["0xEEEEEEEEFFFFFFFF", "0xEEEEEEEEFFFFFFFF", "0x0000000000000000", "0x0000000000000000"],
      "XMM3": ["0xEEEEEEEEFFFFFFFF", "0xEEEEEEEEFFFFFFFF", "0xAAAAAAAABBBBBBBB", "0xAAAAAAAABBBBBBBB"],
      "XMM4": ["0xEEEEEEEEFFFFFFFF", "0xEEEEEEEEFFFFFFFF", "0x0000000000000000", "0x0000000000000000"],
      "XMM5": ["0xEEEEEEEEFFFFFFFF", "0xEEEEEEEEFFFFFFFF", "0xAAAAAAAABBBBBBBB", "0xAAAAAAAABBBBBBBB"],
      "XMM6": ["0xCCCCCCCCDDDDDDDD", "0xCCCCCCCCDDDDDDDD", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

;; Register duplication
vmovapd ymm0, [rdx]
vmovddup ymm1, ymm0
; 128-bit
vmovddup xmm2, xmm0

;; Same register
vmovapd ymm3, ymm0
vmovddup ymm3, ymm3
; 128-bit
vmovapd ymm4, ymm0
vmovddup xmm4, xmm4

;; From memory
vmovddup ymm5, [rdx]
; 128-bit
vmovddup xmm6, [rdx + 8]

hlt

align 32
.data:
db 0xFF, 0xFF, 0xFF, 0xFF, 0xEE, 0xEE, 0xEE, 0xEE, 0xDD, 0xDD, 0xDD, 0xDD, 0xCC, 0xCC, 0xCC, 0xCC
db 0xBB, 0xBB, 0xBB, 0xBB, 0xAA, 0xAA, 0xAA, 0xAA, 0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08
