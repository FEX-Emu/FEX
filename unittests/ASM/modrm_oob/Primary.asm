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

; Primary table
rw3 add, 8, rax
rw3 add, 4, eax
rw3 add, 2, ax
rw3 add, 1, al

w3 lock add, 8, rax
w3 lock add, 4, eax
w3 lock add, 2, ax
w3 lock add, 1, al

rw3 or, 8, rax
rw3 or, 4, eax
rw3 or, 2, ax
rw3 or, 1, al

w3 lock or, 8, rax
w3 lock or, 4, eax
w3 lock or, 2, ax
w3 lock or, 1, al

rw3 adc, 8, rax
rw3 adc, 4, eax
rw3 adc, 2, ax
rw3 adc, 1, al

w3 lock adc, 8, rax
w3 lock adc, 4, eax
w3 lock adc, 2, ax
w3 lock adc, 1, al

rw3 sbb, 8, rax
rw3 sbb, 4, eax
rw3 sbb, 2, ax
rw3 sbb, 1, al

w3 lock sbb, 8, rax
w3 lock sbb, 4, eax
w3 lock sbb, 2, ax
w3 lock sbb, 1, al

rw3 and, 8, rax
rw3 and, 4, eax
rw3 and, 2, ax
rw3 and, 1, al

w3 lock and, 8, rax
w3 lock and, 4, eax
w3 lock and, 2, ax
w3 lock and, 1, al

rw3 xor, 8, rax
rw3 xor, 4, eax
rw3 xor, 2, ax
rw3 xor, 1, al

w3 lock xor, 8, rax
w3 lock xor, 4, eax
w3 lock xor, 2, ax
w3 lock xor, 1, al

rw3 cmp, 8, rax
rw3 cmp, 4, eax
rw3 cmp, 2, ax
rw3 cmp, 1, al

r4 imul, 8, rax, 4
r4 imul, 4, eax, 4
r4 imul, 2, ax, 4

r4 imul, 8, rax, 0x1004
r4 imul, 4, eax, 0x1004
r4 imul, 2, ax, 0x1004

rw3 test, 8, rax
rw3 test, 4, eax
rw3 test, 2, ax
rw3 test, 1, al

rw3 xchg, 8, rax
rw3 xchg, 4, eax
rw3 xchg, 2, ax
rw3 xchg, 1, al

rw3 mov, 8, rax
rw3 mov, 4, eax
rw3 mov, 2, ax
rw3 mov, 1, al

r3 movsxd, 4, rax

; Done
mov rax, 1
hlt
