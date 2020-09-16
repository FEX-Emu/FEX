%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x20"
  }
}
%endif

jmp main

patched_op:
mov rax,-1
ret

main:

; warm up the cache
call patched_op

mov byte [rel patched_op], 0xC3

mov rax, 32
call patched_op

hlt