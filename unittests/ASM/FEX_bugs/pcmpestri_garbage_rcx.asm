%ifdef CONFIG
{
  "RegData": {
      "RAX": ["15"],
      "RCX": ["5"],
      "RDX": ["16"]
  },
  "HostFeatures": ["SSE4.2"]
}
%endif

; Tests a bug that FEX had with pcmpestri where the returned index would leave data in the upper 32-bits of rcx.
; This instruction writes a 32-bit result to rcx with zero extend to 64-bit.
; Test this by writing data in to rcx before the instruction and ensuring it is erased after the fact.

movaps xmm2, [rel .data]
movaps xmm3, [rel .data + 32]

; Unsigned byte character check (lsb, positive polarity)
mov rax, 15 ; Exclude 'l'
mov rdx, 16
mov rcx, 0x4142434445464748

pcmpestri xmm2, xmm3, 0b00000000
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
