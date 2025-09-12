%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "MemoryRegions": {
    "0x100000000": "4096",
    "0x100002000": "4096"
  }
}
%endif

mov r15, 0x100001000
mov r14, 0x100002000
mov rax, 0

%include "modrm_oob_macros.mac"

; Primary Group

w4_size add, 1, byte, 1
w4_size add, 2, word, 1
w4_size add, 4, dword, 1
w4_size add, 8, qword, 1

w4_size lock add, 1, byte, 1
w4_size lock add, 2, word, 1
w4_size lock add, 4, dword, 1
w4_size lock add, 8, qword, 1

w4_size or, 1, byte, 1
w4_size or, 2, word, 1
w4_size or, 4, dword, 1
w4_size or, 8, qword, 1

w4_size lock or, 1, byte, 1
w4_size lock or, 2, word, 1
w4_size lock or, 4, dword, 1
w4_size lock or, 8, qword, 1

w4_size adc, 1, byte, 1
w4_size adc, 2, word, 1
w4_size adc, 4, dword, 1
w4_size adc, 8, qword, 1

w4_size lock adc, 1, byte, 1
w4_size lock adc, 2, word, 1
w4_size lock adc, 4, dword, 1
w4_size lock adc, 8, qword, 1

w4_size sbb, 1, byte, 1
w4_size sbb, 2, word, 1
w4_size sbb, 4, dword, 1
w4_size sbb, 8, qword, 1

w4_size lock sbb, 1, byte, 1
w4_size lock sbb, 2, word, 1
w4_size lock sbb, 4, dword, 1
w4_size lock sbb, 8, qword, 1

w4_size and, 1, byte, 1
w4_size and, 2, word, 1
w4_size and, 4, dword, 1
w4_size and, 8, qword, 1

w4_size lock and, 1, byte, 1
w4_size lock and, 2, word, 1
w4_size lock and, 4, dword, 1
w4_size lock and, 8, qword, 1

w4_size sub, 1, byte, 1
w4_size sub, 2, word, 1
w4_size sub, 4, dword, 1
w4_size sub, 8, qword, 1

w4_size lock sub, 1, byte, 1
w4_size lock sub, 2, word, 1
w4_size lock sub, 4, dword, 1
w4_size lock sub, 8, qword, 1

w4_size xor, 1, byte, 1
w4_size xor, 2, word, 1
w4_size xor, 4, dword, 1
w4_size xor, 8, qword, 1

w4_size lock xor, 1, byte, 1
w4_size lock xor, 2, word, 1
w4_size lock xor, 4, dword, 1
w4_size lock xor, 8, qword, 1

w4_size cmp, 1, byte, 1
w4_size cmp, 2, word, 1
w4_size cmp, 4, dword, 1
w4_size cmp, 8, qword, 1

w4_size rol, 1, byte, 1
w4_size rol, 2, word, 1
w4_size rol, 4, dword, 1
w4_size rol, 8, qword, 1

w4_size ror, 1, byte, 1
w4_size ror, 2, word, 1
w4_size ror, 4, dword, 1
w4_size ror, 8, qword, 1

w4_size rcl, 1, byte, 1
w4_size rcl, 2, word, 1
w4_size rcl, 4, dword, 1
w4_size rcl, 8, qword, 1

w4_size rcr, 1, byte, 1
w4_size rcr, 2, word, 1
w4_size rcr, 4, dword, 1
w4_size rcr, 8, qword, 1

w4_size shl, 1, byte, 1
w4_size shl, 2, word, 1
w4_size shl, 4, dword, 1
w4_size shl, 8, qword, 1

w4_size shr, 1, byte, 1
w4_size shr, 2, word, 1
w4_size shr, 4, dword, 1
w4_size shr, 8, qword, 1

w4_size sar, 1, byte, 1
w4_size sar, 2, word, 1
w4_size sar, 4, dword, 1
w4_size sar, 8, qword, 1

w4_size test, 1, byte, 1
w4_size test, 2, word, 1
w4_size test, 4, dword, 1
w4_size test, 8, qword, 1

w3_size not, 1, byte
w3_size not, 2, word
w3_size not, 4, dword
w3_size not, 8, qword

w3_size lock not, 1, byte
w3_size lock not, 2, word
w3_size lock not, 4, dword
w3_size lock not, 8, qword

w3_size neg, 1, byte
w3_size neg, 2, word
w3_size neg, 4, dword
w3_size neg, 8, qword

w3_size lock neg, 1, byte
w3_size lock neg, 2, word
w3_size lock neg, 4, dword
w3_size lock neg, 8, qword

w3_size mul, 1, byte
w3_size mul, 2, word
w3_size mul, 4, dword
w3_size mul, 8, qword

w3_size imul, 1, byte
w3_size imul, 2, word
w3_size imul, 4, dword
w3_size imul, 8, qword

w3_size div, 1, byte
w3_size div, 2, word
w3_size div, 4, dword
w3_size div, 8, qword

w3_size idiv, 1, byte
w3_size idiv, 2, word
w3_size idiv, 4, dword
w3_size idiv, 8, qword

w3_size inc, 1, byte
w3_size inc, 2, word
w3_size inc, 4, dword
w3_size inc, 8, qword

w3_size lock inc, 1, byte
w3_size lock inc, 2, word
w3_size lock inc, 4, dword
w3_size lock inc, 8, qword

w3_size dec, 1, byte
w3_size dec, 2, word
w3_size dec, 4, dword
w3_size dec, 8, qword

w3_size lock dec, 1, byte
w3_size lock dec, 2, word
w3_size lock dec, 4, dword
w3_size lock dec, 8, qword

w4_size mov, 1, byte, 1
w4_size mov, 2, word, 1
w4_size mov, 4, dword, 1
w4_size mov, 8, qword, 1

; Done
mov rax, 1
hlt
