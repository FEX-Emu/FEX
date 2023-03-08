%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM5":  ["0x0000000000000000", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM6":  ["0xFE02FE02FE02FE02", "0xFE02FE02FE02FE02", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0x7E027E027E027E02", "0x7E027E027E027E02", "0x0000000000000000", "0x0000000000000000"],
    "XMM8":  ["0x7FFF7FFF7FFF7FFF", "0x7FFF7FFF7FFF7FFF", "0x0000000000000000", "0x0000000000000000"],
    "XMM9":  ["0x057306BC07B808B8", "0xBC53BC0EBAE5BA2E", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0xA473A5BCA6B8A7B8", "0x0553070E07E5092E", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovaps xmm0, [rdx]
vmovaps xmm1, [rdx + 16]
vmovaps xmm2, [rdx + 16 * 2]
vmovaps xmm3, [rdx + 16 * 3]
vmovaps xmm4, [rdx + 16 * 4]

; Zero
vpmaddubsw xmm5, xmm0, [rdx]

; -1
vpmaddubsw xmm6, xmm1, [rdx + 16]

; 127
vpmaddubsw xmm7, xmm2, [rdx + 16 * 2]

; 255 and 127
vpmaddubsw xmm8, xmm1, [rdx + 16 * 2]

; Mixture
vpmaddubsw xmm9,  xmm3, [rdx + 16 * 4]
vpmaddubsw xmm10, xmm4, [rdx + 16 * 3]

hlt

align 32
.data:
dq 0x0000000000000000
dq 0x0000000000000000

dq -1
dq -1

dq 0x7F7F7F7F7F7F7F7F
dq 0x7F7F7F7F7F7F7F7F

dq 0x8141824383448445
dq 0x21F223F323F424F5

dq 0xE251E352E453E554
dq 0x71A972A873A774A6
