%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x424446484a4c4e50",
    "RCX": "0x4142434445464748",
    "R8": "0x4142434445464748",
    "R9": "0x4142434445464748"
  }
}
%endif

mov rax, 0x4142434445464748
mov rcx, 0x4142434445464748
mov r8, 0x4142434445464748
mov r9, 0x4142434445464748

lea rbx, [rel .data]
jmp .test
.test:

; add rax, [rbx]
; Real encoding: 0x44, 0x03, 0x03
; Add additional false REX as padding that would convert `rax` to `r8`.
; FEX treated REX prefixes as cumulative at one point.
db 0x44, 0x48, 0x03, 0x03

hlt

align 16
.data:
dq 0x0102030405060708
