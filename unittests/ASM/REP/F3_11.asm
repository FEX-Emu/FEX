%ifdef CONFIG
{
  "RegData": {
    "XMM0":  ["0x0000000045464748", "0x0"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax

mov rax, 0x0
mov [rdx + 8 * 2], rax
mov rax, 0x0
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx + 8 * 0]

; Moves lower 32bits to memory
movss [rdx + 8 * 2], xmm0

; Ensure 128bits weren't written
movapd xmm0, [rdx + 8 * 2]

hlt
