%ifdef CONFIG
{
  "RegData": {
    "RCX": "6",
    "RDI": "0xE0000010"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x6162636465666768
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x0
mov [rdx + 8 * 2], rax

lea rdi, [rdx + 8 * 0]

cld
mov rax, 0x6162636465666768
mov rbx, 0x6162636465666768
mov rcx, 8
cmp rax, rbx

rep scasq

hlt
