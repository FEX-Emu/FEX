%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x0000000000000001", "0x0"],
    "XMM1":  ["0x0000000000030001", "0x0"],
    "XMM2":  ["0x0000000000070001", "0x0"],
    "XMM3":  ["0x0000000000010001", "0x0"]
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

mov rdx, 0xe0000000

; Pos 0
mov rax, 0x0004000300020001
mov [rdx + 8 * 0], rax
mov rax, 0x0008000700060005
mov [rdx + 8 * 1], rax

; Pos 3
mov rax, 0x0001000300020004
mov [rdx + 8 * 2], rax
mov rax, 0x0008000700060005
mov [rdx + 8 * 3], rax

; Pos 7
mov rax, 0x0008000300020004
mov [rdx + 8 * 4], rax
mov rax, 0x0001000700060005
mov [rdx + 8 * 5], rax

; Pos 7 & 3 & 2
; Should return lowest position
mov rax, 0x0008000100010004
mov [rdx + 8 * 6], rax
mov rax, 0x0001000700060005
mov [rdx + 8 * 7], rax

phminposuw xmm0, [rdx + 8 * 0]
phminposuw xmm1, [rdx + 8 * 2]
phminposuw xmm2, [rdx + 8 * 4]
phminposuw xmm3, [rdx + 8 * 6]

hlt
