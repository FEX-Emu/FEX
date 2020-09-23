%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFF",
    "RBX": "0x00",
    "RCX": "0x00",
    "RDX": "0xF0"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x8080808080808080
mov [rdx + 8 * 0], rax
mov rax, 0x0000000000000000
mov [rdx + 8 * 1], rax
mov rax, 0x7070707070707070
mov [rdx + 8 * 2], rax
mov rax, 0x8080808000000000
mov [rdx + 8 * 3], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]
movq mm2, [rdx + 8 * 2]
movq mm3, [rdx + 8 * 3]

mov rax, -1
mov rbx, -1
mov rcx, -1
mov rdx, -1

pmovmskb eax, mm0
pmovmskb ebx, mm1
pmovmskb ecx, mm2
pmovmskb edx, mm3

hlt
