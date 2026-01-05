%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x424446484a4c4e50",
    "RCX": "0x00000000d2af0486",
    "R8": "0"
  }
}
%endif

mov rax, 0x4142434445464748
mov rcx, 0x4142434445464748

mov r8, 0

lea rbx, [rel .data]
jmp .test
.test:

; adcx rax, [rbx]
; Real encoding: 0x66, 0x48, 0x0f, 0x38, 0xf6, 0x03
; Add a dummy REX prefix that enables everything. Really mess up FEX's cumulative usage.
db 0x4f, 0x66, 0x48, 0x0f, 0x38, 0xf6, 0x03

; crc32 ecx, dword [rbx]
; Real encoding: 0xf2, 0x0f, 0x38, 0xf1, 0x0b
; Add a dummy rex encoding with the widening bit set.
; If FEX parsed this incorrectly, then it converts the crc in to a 64-bit version.
db 0x48, 0xf2, 0x0f, 0x38, 0xf1, 0x0b

hlt

align 16
.data:
dq 0x0102030405060708
