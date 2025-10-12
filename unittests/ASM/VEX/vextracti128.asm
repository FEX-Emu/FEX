%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM2":  ["0xAAAAAAAABBBBBBBB", "0xCCCCCCCCDDDDDDDD", "0x0000000000000000", "0x0000000000000000"],
    "XMM3":  ["0xEEEEEEEEFFFFFFFF", "0x9999999988888888", "0x0000000000000000", "0x0000000000000000"],
    "XMM14": ["0x1111111122222222", "0x3333333344444444", "0xAAAABBBBCCCCDDDD", "0xEEEEFFFF99998888"],
    "XMM15": ["0x5555555566666666", "0x7777777788888888", "0x4444333322221111", "0x8888777766665555"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd ymm0, [rdx]
vmovapd ymm1, [rdx + 32]

; Load junk and overwrite register
vmovapd ymm2, [rdx + 32]
vmovapd ymm3, [rdx + 32]
vextracti128 xmm2, ymm0, 0
vextracti128 xmm3, ymm0, 1

; Store into memory
vextracti128 [rel .scratch1], ymm1, 0
vextracti128 [rel .scratch2], ymm1, 1
vmovapd ymm14, [rel .scratch1]
vmovapd ymm15, [rel .scratch2]

hlt

align 4096
.data:
dq 0xAAAAAAAABBBBBBBB
dq 0xCCCCCCCCDDDDDDDD
dq 0xEEEEEEEEFFFFFFFF
dq 0x9999999988888888

dq 0x1111111122222222
dq 0x3333333344444444
dq 0x5555555566666666
dq 0x7777777788888888

.scratch1:
dq 0x8888777766665555
dq 0x4444333322221111
dq 0xAAAABBBBCCCCDDDD
dq 0xEEEEFFFF99998888

.scratch2:
dq 0xEEEEFFFF99998888
dq 0xAAAABBBBCCCCDDDD
dq 0x4444333322221111
dq 0x8888777766665555
