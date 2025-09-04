%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0xFFFFFFFFFFFF42FF", "0xFFFFFFFFFFFFFFFF"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

mov rsi, 0xe0000000

mov rax, -1
mov [rsi + 8 * 0], rax
mov [rsi + 8 * 1], rax

movaps xmm0, [rsi + 16 * 0]

mov rcx, 0
mov edi, 0x42
; This tests a frontend decoder bug in FEX
; FEX thought this was ch
pinsrb xmm0, edi, 0x01

hlt
