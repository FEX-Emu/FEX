%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFF3800",
    "RBX": "0xFFFFFFFFFFFF0000"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax

mov rax, -1
mov rbx, -1
fnstsw [rdx + 8 * 1]

fld dword [rdx + 8 * 0]
fnstsw [rdx + 8 * 2]
mov ax, word [rdx + 8 * 2]
mov bx, word [rdx + 8 * 1]

hlt
