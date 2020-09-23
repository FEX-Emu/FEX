%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x41424344FFFFFFFF",
    "RCX": "0xFFFFFFFFFFFFFFFF"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x8080808080808080
mov [rdx + 8 * 1], rax
mov rax, 0x8080808000000000
mov [rdx + 8 * 2], rax
mov rax, 0
mov [rdx + 8 * 3], rax
mov rax, -1
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax
mov [rdx + 8 * 6], rax

movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]
movq mm2, [rdx + 8 * 2]
movq mm3, [rdx + 8 * 3]

lea rdi, [rdx + 8 * 4]
maskmovq mm0, mm1

lea rdi, [rdx + 8 * 5]
maskmovq mm0, mm2

lea rdi, [rdx + 8 * 6]
maskmovq mm0, mm3

mov rax, qword [rdx + 8 * 4]
mov rbx, qword [rdx + 8 * 5]
mov rcx, qword [rdx + 8 * 6]

hlt
