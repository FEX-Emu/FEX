%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0F"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000

mov rbx, 0xFFFFFFFFFFFFFF00
mov [r15 + 8 * 0], rbx
mov rbx, -1
mov [r15 + 8 * 1], rbx

mov rax, 0
mov rcx, 0x10
cmp byte [r15 + rcx - 1], 0

jmp .head

.top:

add rax, 1
cmp byte [r15 + rcx - 1], 0

.head:

loopne .top

hlt
