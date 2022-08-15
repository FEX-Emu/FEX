%ifdef CONFIG
{
  "RegData": {
      "XMM1": ["0xA5E1EC3918BE0C95", "0xA3F7BF0143303AFB"],
      "XMM2": ["0x6868C3F3AAED56E0", "0xF0FCE9E294E6E6DE"],
      "XMM3": ["0xA8B3BD15FA04D6D7", "0xDE761956A1F750B1"],
      "XMM4": ["0xCFBBDFA4E5E4712D", "0x76AD0D46447127D3"],
      "XMM5": ["0x9BDCA44510E70C65", "0x4474BFAA2B70A524"]
  },
  "HostFeatures": ["SHA"]
}
%endif

lea rdx, [rel .data]

; We use XMM2 as the hypothetical E state
; This should not change across multiple invocations
; of SHA1RNDS4
movaps xmm2, [rdx + 16 * 1]

; With imm = 0
movaps xmm1, [rdx + 16 * 0]
sha1rnds4 xmm1, xmm2, 0

; With imm = 1
movaps xmm3, [rdx + 16 * 0]
sha1rnds4 xmm3, xmm2, 1

; With imm = 2
movaps xmm4, [rdx + 16 * 0]
sha1rnds4 xmm4, xmm2, 2

; With imm = 3
movaps xmm5, [rdx + 16 * 0]
sha1rnds4 xmm5, xmm2, 3

hlt

align 16
.data:
db 0xe0, 0xfc, 0x2b, 0xa1, 0x06, 0x4f, 0x6c, 0xa7, 0x0f, 0x06, 0x6a, 0x1e, 0x7f, 0x76, 0x80, 0x9b
db 0xe0, 0x56, 0xed, 0xaa, 0xf3, 0xc3, 0x68, 0x68, 0xde, 0xe6, 0xe6, 0x94, 0xe2, 0xe9, 0xfc, 0xf0
