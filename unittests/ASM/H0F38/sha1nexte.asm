%ifdef CONFIG
{
  "RegData": {
      "XMM1": ["0x6868C3F3AAED56E0", "0xD7DD078194E6E6DE"],
      "XMM2": ["0x6868C3F3AAED56E0", "0xF0FCE9E294E6E6DE"],
      "XMM3": ["0xB3912B1060F8BB11", "0xF434B8921111DB88"],
      "XMM4": ["0x7557153EE4DF1BEF", "0x55037BB81B0233CC"],
      "XMM5": ["0x2847E41AD8743E97", "0x5334516CA8F9D3BD"],
      "XMM6": ["0x7557153EE4DF1BEF", "0xEC058BF81B0233CC"],
      "XMM7": ["0x2847E41AD8743E97", "0x78DCF995A8F9D3BD"],
      "XMM10": ["0x66F7F348CF3B457C", "0x7E8CA3D3F3370A9E"],
      "XMM11": ["0x6868C3F3AAED56E0", "0x59FAD9A294E6E6DE"],
      "XMM12": ["0x82086C867D3D9714", "0x4990CDDF4B032437"],
      "XMM13": ["0x0000000000000000", "0xA906304000000000"]
  },
  "HostFeatures": ["SHA"]
}
%endif

lea rdx, [rel .data]

movaps xmm1, [rdx + 16 * 0]
movaps xmm2, [rdx + 16 * 1]

; XMM1 is expected to contain the same value as XMM2
; but with the top 32-bit word set to an equivalent
; of XMM2[127:96] + (XMM1[127:96] ROL 30)

sha1nexte xmm1, xmm2

; Now do the patterns present in some crypto libraries
; to excercise the SHA1 peephole optimizations.
movaps xmm3, [rdx + 16 * 0]
movaps xmm4, [rdx + 16 * 1]
movaps xmm6, [rdx + 16 * 1]
movaps xmm7, [rdx + 16 * 0]
movaps xmm8, [rdx + 16 * 1]
movaps xmm9, [rdx + 16 * 0]

movaps xmm5, xmm3
sha1rnds4 xmm3, xmm4, 0
sha1nexte xmm5, xmm7
sha1msg1 xmm6, xmm7
pxor xmm6, xmm8
sha1msg2 xmm6, xmm9

movaps xmm4, xmm3
sha1rnds4 xmm3, xmm5, 1
sha1nexte xmm4, xmm6
sha1msg1 xmm7, xmm8
pxor xmm7, xmm9
sha1msg2 xmm7, xmm6

movaps xmm5, xmm3
sha1rnds4 xmm3, xmm4, 3
sha1nexte xmm5, xmm7

; Now cases where the peephole optimization
; must NOT be applied for correctness.
movaps xmm10, [rdx + 16 * 0]
sha1rnds4 xmm10, xmm2, 0
movaps xmm11, xmm10
sha1rnds4 xmm10, xmm2, 1
sha1rnds4 xmm10, xmm2, 2
sha1nexte xmm11, xmm2

movaps xmm12, [rdx + 16 * 0]
sha1rnds4 xmm12, xmm2, 0
movaps xmm13, xmm12
sha1rnds4 xmm12, xmm2, 1
sha1nexte xmm13, xmm2
pxor xmm13, xmm2
sha1rnds4 xmm12, xmm13, 2

hlt

align 16
.data:
db 0xe0, 0xfc, 0x2b, 0xa1, 0x06, 0x4f, 0x6c, 0xa7, 0x0f, 0x06, 0x6a, 0x1e, 0x7f, 0x76, 0x80, 0x9b
db 0xe0, 0x56, 0xed, 0xaa, 0xf3, 0xc3, 0x68, 0x68, 0xde, 0xe6, 0xe6, 0x94, 0xe2, 0xe9, 0xfc, 0xf0
