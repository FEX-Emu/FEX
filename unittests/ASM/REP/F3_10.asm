%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x4142434465666768", "0x5152535455565758"],
    "XMM1":  ["0x0000000065666768", "0x0"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445432748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x6162633265666768
mov [rdx + 8 * 2], rax
mov rax, 0x7172737475767778
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]
movapd xmm1, [rdx + 8 * 0]
movapd xmm2, [rdx + 8 * 2]

; Moves 32bits to lower bits
; Doesn't effect upper bits
movss xmm0, xmm2

; Moves 32bits to the lower bits
; Zeroes the upper bits
movss xmm1, [rdx + 8 * 2]

hlt
