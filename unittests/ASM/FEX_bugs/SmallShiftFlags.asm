%ifdef CONFIG
{
  "RegData": {
    "R8":  "0x246",
    "R9":  "0x246",
    "R10": "0x246",
    "R11": "0x246",
    "R12": "0x246",
    "R13": "0x246",
    "R14": "0x246",
    "R15": "0x246"
  }
}
%endif

; FEX-Emu had a bug where 8-bit and 16-bit shifts with large offsets would calculate flags incorrectly.
mov rsp, 0xe000_1000
mov rax, 0x8234fdb482345679

; Large shift that is larger than the element size but smaller than mask limit of 0x1F
mov rcx, 0x5152535455565714
jmp .test
.test:

; Ensure that 16-bit shift updates flags correctly.
shl ax, cl
pushfq
pop r15
; Clear OF and AF since those are undefined
and r15, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test2
.test2:

sar ax, cl
pushfq
pop r14
; Clear OF and AF since those are undefined
and r14, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test3
.test3:

shl ax, 0x14
pushfq
pop r13
; Clear OF and AF since those are undefined
and r13, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test4
.test4:

sar ax, 0x14
pushfq
pop r12
; Clear OF and AF since those are undefined
and r12, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test5
.test5:

; Ensure that 8-bit shift updates flags correctly.
shl al, cl
pushfq
pop r11
; Clear OF and AF since those are undefined
and r11, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test6
.test6:

sar al, cl
pushfq
pop r10
; Clear OF and AF since those are undefined
and r10, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test7
.test7:
shl al, 0x14
pushfq
pop r9
; Clear OF and AF since those are undefined
and r9, ~((1 << 11) | (1 << 4))

; Set up the next test.
mov rax, 0x8234fdb482345679
jmp .test8
.test8:

sar al, 0x14
pushfq
pop r8
; Clear OF and AF since those are undefined
and r8, ~((1 << 11) | (1 << 4))

hlt
