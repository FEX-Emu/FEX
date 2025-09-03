%ifdef CONFIG
{
  "RegData": {
    "XMM5":  ["0x0000000000000048", "0x0000000000000047"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif


; FEX-Emu had a bug where a vector load that was using SIB addressing would overflow to larger than what ARM could encode.
; Test that here.
; Original bug came from the Darwinia Linux binary from function `HUF_readDTableX1_wksp`

mov rbx, 0
lea r15, [rel .data - 0x3d4]

; Break the block
jmp .test
.test:

pmovzxbq xmm5, word [rbx+r15+0x3d4]

hlt

.data:
dq 0x4142434445464748, 0x5152535455565758
