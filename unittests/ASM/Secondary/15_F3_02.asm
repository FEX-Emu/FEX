%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748"
  }
}
%endif

mov rdx, 0xe0000000
wrfsbase rdx

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax

mov rax, -1
mov rax, qword [fs:0]

hlt
