%ifdef CONFIG
{
  "Match": "All",
  "RegData": {
    "RAX": "0x20"
  }
}
%endif


mov rax, 32

; patch mov rax,... to nops
mov byte [rel patched_op + 0], 0x90
mov byte [rel patched_op + 1], 0x90
mov byte [rel patched_op + 2], 0x90
mov byte [rel patched_op + 3], 0x90
mov byte [rel patched_op + 4], 0x90
mov byte [rel patched_op + 5], 0x90
mov byte [rel patched_op + 6], 0x90
mov byte [rel patched_op + 7], 0x90
mov byte [rel patched_op + 8], 0x90
mov byte [rel patched_op + 9], 0x90

patched_op:
mov rax,0xFABCFABCFABC0123 ; 10 bytes long

hlt