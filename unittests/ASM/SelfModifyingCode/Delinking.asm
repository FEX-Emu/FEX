%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x20"
  }
}
%endif

mov rcx, 2
mov rbx, 1
jmp main

patched_op:
db 0x48, 0xc7, 0xc0, 0xff, 0xff, 0xff, 0xff
dec rcx
jmp main

main:

; warm up the cache
cmp rcx, 0
jg patched_op

; should the text exit?
cmp rbx, 0
je end

; patch mov rax, -1 to nops
mov byte [rel patched_op + 0], 0x90
mov byte [rel patched_op + 1], 0x90
mov byte [rel patched_op + 2], 0x90
mov byte [rel patched_op + 3], 0x90
mov byte [rel patched_op + 4], 0x90
mov byte [rel patched_op + 5], 0x90
mov byte [rel patched_op + 6], 0x90

mov rax, 32
mov rcx, 2
mov rbx, 0
jmp main

end:
hlt