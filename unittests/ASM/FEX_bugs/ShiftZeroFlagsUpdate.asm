%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000082345679",
    "R12": "0x0000000000000202",
    "R13": "0x0000000000000202",
    "R14": "0x0000000000000202",
    "R15": "0x0000000000000202"
  }
}
%endif

; FEX-Emu has a bug where a shift by zero was updating flags.
; x86 shift by zero must not update flags.
mov rsp, 0xe000_1000
mov rax, 0x8234fdb482345679
mov rcx, 0x51525354555657E0
mov rbx, 0
mov rdx, 0
push rbx
popfq

jmp .test
.test:

; Ensure that a 32-bit shift of zero doesn't update flags.
shl eax, cl
pushfq
pop r15

; Set up the next test.
mov rdx, 0
pushfq
jmp .test2
.test2:

sar eax, cl
pushfq
pop r14

; Set up the next test.
mov rdx, 0
pushfq
jmp .test3
.test3:

shl eax, 0xE0
pushfq
pop r13


; Set up the next test.
mov rdx, 0
pushfq
jmp .test4
.test4:

sar eax, 0xE0
pushfq
pop r12

hlt
