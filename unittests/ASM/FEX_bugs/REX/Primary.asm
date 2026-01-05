%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464e50",
    "RCX": "0x000000004a4c4e50",
    "R8": "0x4142434445464748",
    "R9": "0x4142434445464748"
  }
}
%endif

; FEX-Emu had a bug where REX was not correctly ignored if it was placed at the wrong location.
; "Wrong" means it wasn't encoded just before the opcode byte.
; This can be done for multiple reasons, either padding or anti-emulation.
mov rax, 0x4142434445464748
mov rcx, 0x4142434445464748
mov r8, 0x4142434445464748
mov r9, 0x4142434445464748

lea rbx, [rel .data]
jmp .test
.test:

; add r8w, [rbx]
; Real encoding: 0x66, 0x44, 0x03, 0x03
; Swap operand-size override and REX. Converts r8 to rax, and stays a 16-bit operation.
db 0x44, 0x66, 0x03, 0x03

; add r9, [rbx]
; Real encoding: 0x4c, 0x03, 0x0b
; Add extraneous segment-overide between REX prefix and op, changes r9 to rcx, and 64-bit to 32-bit.
db 0x4c, 0x2e, 0x03, 0x0b
hlt

align 16
.data:
dq 0x0102030405060708
