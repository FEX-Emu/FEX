%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0": ["0x00000000000060a0", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM1": ["0x00000000000060a0", "0x0000000000000000", "0xFFFFFFFFFFFFFFFF", "0xEEEEEEEEEEEEEEEE"],
    "XMM2": ["0x6463626144434241", "0x6C6B6A694C4B4A49", "0xFFFFFFFFFFFFFFFF", "0xEEEEEEEEEEEEEEEE"],
    "XMM3": ["0x4120492065726548", "0x6C4C612759202C6D", "0xDDDDDDDDDDDDDDDD", "0xCCCCCCCCCCCCCCCC"]
  }
}
%endif

vmovaps ymm0, [rel .data]
vmovaps ymm1, [rel .data + (1 * 32)]
vmovaps ymm2, [rel .data]
vmovaps ymm3, [rel .data + (1 * 32)]

mov rax, 15 ; Exclude 'l'
mov rdx, 16

pcmpestrm xmm2, xmm3, 0b00000000
vmovaps ymm1, ymm0
vpcmpestrm xmm2, xmm3, 0b00000000

hlt

align 32
.data:
dq 0x6463626144434241 ; "ABCDabcd"
dq 0x6C6B6A694C4B4A49 ; "IJKLijkl"
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE

dq 0x4120492065726548 ; "Here I A"
dq 0x6C4C612759202C6D ; "m, Y'aLl"
dq 0xDDDDDDDDDDDDDDDD
dq 0xCCCCCCCCCCCCCCCC
