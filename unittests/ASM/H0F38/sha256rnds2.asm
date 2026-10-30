%ifdef CONFIG
{
  "RegData": {
      "XMM1": ["0x97D4574EE323773D", "0xA934C32F562D8E88"],
      "XMM2": ["0x6868C3F3AAED56E0", "0xF0FCE9E294E6E6DE"],
      "XMM3": ["0x97D4574EE323773D", "0xA934C32F562D8E88"],
      "XMM4": ["0x12EC0B542095D0D2", "0x6BDF510B9C287D67"],
      "XMM5": ["0x97D4574EE323773D", "0xA934C32F562D8E88"],
      "XMM6": ["0x12EC0B542095D0D2", "0x6BDF510B9C287D67"],
      "XMM0": ["0xBB5A9A6046F6F8D0", "0x39C210E660786A57"],
      "XMM7": ["0x8013E21F99A0FC9C", "0x0F9B1B966B7CACFE"],
      "XMM9": ["0x499793854A5D5652", "0xB519225A80462AB4"],
      "XMM10": ["0x3E4B561875127AA5", "0x6C5ED7074C69CB40"],
      "XMM12": ["0xCF74894E65DDAE89", "0xD0FE60BF24CA8913"],
      "XMM13": ["0xCC23CC32EED51377", "0xE4121F2F81CEFB11"]
  },
  "HostFeatures": ["SHA"]
}
%endif

lea rdx, [rel .data]

movaps xmm0, [rdx + 16 * 2]
movaps xmm1, [rdx + 16 * 0]
movaps xmm2, [rdx + 16 * 1]

sha256rnds2 xmm1, xmm2

; Cribbed from OpenSSL, they use this idiom
; and this is the pattern we are looking for in the peephole.
movaps xmm3, [rdx + 16 * 0]
movaps xmm4, [rdx + 16 * 1]
sha256rnds2 xmm3, xmm4
pshufd xmm0, xmm0, 0xe
sha256rnds2 xmm4, xmm3

; We also look for this pattern.
movaps xmm0, [rdx + 16 * 2]
movaps xmm5, [rdx + 16 * 0]
movaps xmm6, [rdx + 16 * 1]
sha256rnds2 xmm5, xmm6
psrldq xmm0, 8
sha256rnds2 xmm6, xmm5

; Now test that the peephole doesn't break anything
; if weird register aliasing is going on.
movaps xmm0, [rdx + 16 * 2]
movaps xmm10, [rdx + 16 * 0]
sha256rnds2 xmm10, xmm10
pshufd xmm0, xmm0, 0xe
sha256rnds2 xmm10, xmm10

movaps xmm0, [rdx + 16 * 2]
movaps xmm11, [rdx + 16 * 0]
sha256rnds2 xmm11, xmm0
pshufd xmm0, xmm0, 0xe
sha256rnds2 xmm0, xmm11
movaps xmm13, xmm0

movaps xmm0, [rdx + 16 * 2]
movaps xmm12, [rdx + 16 * 1]
sha256rnds2 xmm0, xmm12
pshufd xmm0, xmm0, 0xe
sha256rnds2 xmm12, xmm0

movaps xmm0, [rdx + 16 * 2]
movaps xmm7, [rdx + 16 * 0]
sha256rnds2 xmm7, xmm7
movaps xmm9, [rdx + 16 * 0]
sha256rnds2 xmm9, xmm0
movaps xmm8, [rdx + 16 * 1]
sha256rnds2 xmm0, xmm8

hlt

align 16
.data:
db 0xe0, 0xfc, 0x2b, 0xa1, 0x06, 0x4f, 0x6c, 0xa7, 0x0f, 0x06, 0x6a, 0x1e, 0x7f, 0x76, 0x80, 0x9b
db 0xe0, 0x56, 0xed, 0xaa, 0xf3, 0xc3, 0x68, 0x68, 0xde, 0xe6, 0xe6, 0x94, 0xe2, 0xe9, 0xfc, 0xf0
db 0xc7, 0xcd, 0x73, 0xec, 0x95, 0xd6, 0x6f, 0x6a, 0xbb, 0xae, 0xf2, 0xbb, 0x27, 0xb9, 0xa1, 0xdd
