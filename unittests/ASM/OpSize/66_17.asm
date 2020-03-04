%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x5152535455565758",
    "RBX": "0x0",
    "XMM0": ["0x4142434445464748", "0x5152535455565758"]
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
mov [rdx + 8 * 3], rax

; Preload
movupd xmm0, [rdx]

movhpd [rdx + 8 * 2], xmm0

mov rax, [rdx + 8 * 2]
mov rbx, [rdx + 8 * 3] ;Ensure this wasn't overwritten


hlt
