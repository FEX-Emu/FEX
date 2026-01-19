%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000080050033",
    "RBX": "0x4142434480050033",
    "RCX": "0x4142434445460033",
    "RDX": "0x4142434445460033",
    "RDI": "0x0000000080050033",
    "RSP": "0x0000000080050033",
    "RBP": "0x0000000080050033",
    "R8":  "0x4142434445460033",
    "R9":  "0x4142434445460033",
    "R10": "0x4142434445460033"
  }
}
%endif

mov rax, 0x4142434445464748
mov rbx, 0x4142434445464748
mov rcx, 0x4142434445464748
mov rdx, 0x4142434445464748
mov rsi, 0xe000_0000
mov [rsi], rdx

mov rdi, 0x4142434445464748
mov rsp, 0x4142434445464748
mov rbp, 0x4142434445464748
mov r8, 0x4142434445464748
mov r9, 0x4142434445464748
mov r10, 0x4142434445464748

smsw rax
smsw ebx
smsw cx

smsw [rsi]
mov rdx, [rsi]

; operand-size override prefix
; Nasm complains if o16 is used
; `warning: invalid operand size prefix o16, must be o64`
db 0x66
smsw rdi
repe smsw rsp
repne smsw rbp

db 0x66
smsw r8w
repe smsw r9w
repne smsw r10w

hlt
