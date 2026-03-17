%ifdef CONFIG
{
  "RegData": {
    "R12": "0x55",
    "R13": "0x890",
    "R14": "0x55"
  }
}
%endif

mov rsi, 0xe0000080
mov rsp, 0xe0001000

; Zero shift amount
xor ecx, ecx

; Zero all flags
xor eax, eax
push rax
popfq

mov r8b, 255
mov r10b, 127
mov r11b, 1


add r8b, r11b ; Sets CF, ZF, PF, AF, zeroes OF, SF
; Shift by zero, flags should be unaffected
; This tests that we didn't optimize away the flag calculations of the add
shl rax, cl

; Ensure we can't predict the next block
lea rdi, [rel .next]
mov [rsi - 8], rdi
jmp [rsi - 8]

.next:
pushfq
pop r12

; Mask with flags we care about
and r12, 0x8d5

add r10b, r11b ; Sets OF, SF, AF, zeroes ZF, CF, PF
shr rax, cl

lea rdi, [rel .next2]
mov [rsi - 8], rdi
jmp [rsi - 8]

.next2:
pushfq
pop r13
and r13, 0x8d5

mov r8b, 255
add r8b, r11b ; Sets CF, ZF, PF, AF, zeroes OF, SF
sar rax, cl

lea rdi, [rel .next3]
mov [rsi - 8], rdi
jmp [rsi - 8]

.next3:
pushfq
pop r14
and r14, 0x8d5

hlt