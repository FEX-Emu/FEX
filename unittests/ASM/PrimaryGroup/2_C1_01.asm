%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434491514748",
    "RBX": "0x51525354155595D6",
    "RCX": "0x195999da185898d9"
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
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax

ror  word [rdx + 8 * 0 + 2], 0x62
ror dword [rdx + 8 * 1 + 0], 0x62
ror qword [rdx + 8 * 2 + 0], 0x62

mov rax, [rdx + 8 * 0]
mov rbx, [rdx + 8 * 1]
mov rcx, [rdx + 8 * 2]

hlt
