%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4748",
    "RBX": "0x4546",
    "RCX": "0x4344",
    "RDX": "0x4142",
    "RSI": "0x4748",
    "RDI": "0x4546",
    "RBP": "0x4344",
    "RSP": "0x4142"
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

movq mm0, [rdx + 8 * 0]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1

pextrw eax, mm0, 0
pextrw ebx, mm0, 1
pextrw ecx, mm0, 2
pextrw edx, mm0, 3
pextrw esi, mm0, 4
pextrw edi, mm0, 5
pextrw ebp, mm0, 6
pextrw esp, mm0, 7


hlt
